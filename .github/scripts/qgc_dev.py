#!/usr/bin/env python3
"""Plan, verify and promote the single native qgc-dev image."""

from __future__ import annotations

import argparse
import base64
import fnmatch
import hashlib
import json
import os
import re
import subprocess
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

IMAGE = "ghcr.io/mavlink/qgc-dev"
SOURCE = "https://github.com/mavlink/qgroundcontrol"
BAKE = "deploy/docker/docker-bake.hcl"
PLATFORMS = {
    "linux/amd64": ("linux-x64-builder", "ubuntu-24.04"),
    "linux/arm64": ("linux-arm64-builder", "ubuntu-24.04-arm"),
}
IMAGE_INPUTS = (
    ".github/workflows/qgc-dev.yml",
    ".github/actions/qgc-dev/**",
    ".github/scripts/qgc_dev.py",
    ".github/build-config.json",
    ".github/scripts/ccache_helper.py",
    ".github/scripts/ci_bootstrap.py",
    ".devcontainer/**",
    ".dockerignore",
    "deploy/docker/Dockerfile",
    "deploy/docker/docker-bake.hcl",
    "deploy/docker/install_analysis.py",
    "deploy/docker/smoke_dev.py",
    "deploy/docker/lib/**",
    "deploy/docker/entrypoint.*",
    "tools/setup/read_config.py",
    "tools/setup/install_qt.py",
    "tools/setup/install_dependencies/**",
    "tools/common/**",
    "tools/qgc_tools/**",
    "tools/_bootstrap.py",
    "tools/pyproject.toml",
    "tools/uv.lock",
)
DIGEST = re.compile(r"sha256:[0-9a-f]{64}")


def command(*args: str) -> str:
    return subprocess.check_output(args, text=True).strip()


def meaningful(paths: list[str]) -> bool:
    return any(fnmatch.fnmatchcase(path, pattern) for path in paths for pattern in IMAGE_INPUTS)


def release_source(tag: str) -> str:
    if not re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", tag):
        raise ValueError("Expected an exact stable QGC vMAJOR.MINOR.PATCH tag")
    release = json.loads(
        command("gh", "api", f"repos/{os.environ['GITHUB_REPOSITORY']}/releases/tags/{tag}")
    )
    if (
        release["tag_name"] != tag
        or release["draft"]
        or release["prerelease"]
        or not release.get("published_at")
    ):
        raise ValueError("The requested tag is not an actual published stable release")
    # Resolve tags afresh, including on retry, so moved tags cannot reuse an old checkout.
    subprocess.run(
        ["git", "fetch", "--force", "origin", f"refs/tags/{tag}:refs/tags/{tag}"], check=True
    )
    return command("git", "rev-parse", f"refs/tags/{tag}^{{commit}}")


def bake_matrix() -> dict:
    target = json.loads(command("docker", "buildx", "bake", "-f", BAKE, "--print"))["target"][
        "qgc-dev"
    ]
    platforms = target["platforms"]
    if len(platforms) != 2 or set(platforms) != set(PLATFORMS) or target["target"] != "qgc-dev":
        raise ValueError("Bake must define one qgc-dev target for both native platforms")
    return {
        "include": [
            {
                "platform": p,
                "pool": PLATFORMS[p][0],
                "hosted": PLATFORMS[p][1],
                "scope": p.replace("/", "-"),
            }
            for p in platforms
        ]
    }


def plan() -> None:
    tag = os.environ.get("RELEASE_TAG", "")
    event = os.environ["GITHUB_EVENT_NAME"]
    upstream = os.environ["GITHUB_REPOSITORY"] == "mavlink/qgroundcontrol"
    source = release_source(tag) if tag else command("git", "rev-parse", "HEAD")
    publish = upstream and bool(
        tag or (event == "push" and os.environ["GITHUB_REF"] == "refs/heads/master")
    )
    if tag:
        subprocess.run(["git", "checkout", "--detach", source], check=True)
    if not Path(BAKE).is_file():
        raise ValueError("Released source does not contain qgc-dev; never backfill from master")
    output = {
        "source": source,
        "tag": tag or ("latest" if publish else ""),
        "publish": str(publish).lower(),
        "matrix": json.dumps(bake_matrix()),
    }
    with Path(os.environ["GITHUB_OUTPUT"]).open("a") as stream:
        for key, value in output.items():
            stream.write(f"{key}={value}\n")


def smoke() -> None:
    platform = os.environ["NATIVE_PLATFORM"]
    if platform not in PLATFORMS:
        raise ValueError(f"Unsupported platform {platform}")
    native = command("docker", "info", "--format", "{{.Architecture}}")
    if (
        native
        not in {"linux/amd64": ("x86_64", "amd64"), "linux/arm64": ("aarch64", "arm64")}[platform]
    ):
        raise ValueError(f"Refusing emulated build: {native} runner for {platform}")
    publish = os.environ.get("PUBLISH_IMAGE") == "true"
    digest = ""
    ref = "qgc-dev:local"
    if publish:
        digest = json.loads(os.environ["BAKE_METADATA"])["qgc-dev"]["containerimage.digest"]
        if not DIGEST.fullmatch(digest):
            raise ValueError("Invalid image digest")
        ref = f"{IMAGE}@{digest}"
        subprocess.run(["docker", "pull", ref], check=True)
    image = json.loads(command("docker", "image", "inspect", ref))[0]
    if f"{image['Os']}/{image['Architecture']}" != platform:
        raise ValueError("Image platform does not match the native runner")
    if (
        image["Config"]["Labels"].get("org.opencontainers.image.revision")
        != os.environ["SOURCE_REVISION"]
    ):
        raise ValueError("Image source revision does not match the checkout")
    subprocess.run(
        [
            "docker",
            "run",
            "--rm",
            "--network=none",
            "--volume",
            f"{Path.cwd()}:/source:ro",
            ref,
            "python3",
            "/source/deploy/docker/smoke_dev.py",
        ],
        check=True,
    )
    if publish:
        Path("qgc-dev-digest.json").write_text(
            json.dumps(
                {
                    "platform": platform,
                    "digest": digest,
                    "source": os.environ["SOURCE_REVISION"],
                }
            )
        )


class Registry:
    """Read and hash-verify OCI documents, allowing only genuine 404s as absent tags."""

    def __init__(self) -> None:
        headers = {}
        if os.environ.get("GH_TOKEN"):
            credentials = f"{os.environ['GITHUB_ACTOR']}:{os.environ['GH_TOKEN']}".encode()
            headers["Authorization"] = "Basic " + base64.b64encode(credentials).decode()
        request = urllib.request.Request(
            "https://ghcr.io/token?service=ghcr.io&scope=repository:mavlink/qgc-dev:pull",
            headers=headers,
        )
        with urllib.request.urlopen(request, timeout=60) as response:
            self.token = json.load(response)["token"]

    def read(self, ref: str, *, blob: bool = False) -> tuple[dict, str]:
        kind = "blobs" if blob else "manifests"
        request = urllib.request.Request(
            f"https://ghcr.io/v2/mavlink/qgc-dev/{kind}/{urllib.parse.quote(ref, safe=':')}",
            headers={
                "Authorization": f"Bearer {self.token}",
                "Accept": ", ".join(
                    (
                        "application/vnd.oci.image.index.v1+json",
                        "application/vnd.oci.image.manifest.v1+json",
                        "application/vnd.docker.distribution.manifest.list.v2+json",
                        "application/vnd.docker.distribution.manifest.v2+json",
                    )
                ),
            },
        )
        with urllib.request.urlopen(request, timeout=60) as response:
            payload = response.read()
        digest = "sha256:" + hashlib.sha256(payload).hexdigest()
        if ref.startswith("sha256:") and ref != digest:
            raise ValueError("Registry content digest mismatch")
        return json.loads(payload), digest

    def verify_image(self, digest: str, platform: str, source: str) -> None:
        manifest, _ = self.read(digest)
        config, _ = self.read(manifest["config"]["digest"], blob=True)
        labels = config["config"].get("Labels", {})
        if (
            f"{config['os']}/{config['architecture']}" != platform
            or labels.get("org.opencontainers.image.revision") != source
            or labels.get("org.opencontainers.image.source") != SOURCE
        ):
            raise ValueError("Registry image platform/source mismatch")

    def verify_index(self, index: dict, platforms: set[str], source: str) -> None:
        if index.get("mediaType") != "application/vnd.oci.image.index.v1+json":
            raise ValueError("Public tag must be one OCI index")
        manifests = index.get("manifests", [])
        actual = [f"{m['platform']['os']}/{m['platform']['architecture']}" for m in manifests]
        if len(actual) != len(platforms) or set(actual) != platforms:
            raise ValueError("Index must contain exactly both native platforms")
        for manifest, platform in zip(manifests, actual, strict=True):
            self.verify_image(manifest["digest"], platform, source)


def collect_digests(directory: Path, platforms: set[str], source: str) -> dict[str, str]:
    results = {}
    for path in directory.glob("*/qgc-dev-digest.json"):
        record = json.loads(path.read_text())
        platform = record["platform"]
        if (
            platform in results
            or record["source"] != source
            or not DIGEST.fullmatch(record["digest"])
        ):
            raise ValueError("Duplicate platform or invalid source/digest record")
        results[platform] = record["digest"]
    if set(results) != platforms:
        raise ValueError("Both successful platform digests are required")
    return results


def publish() -> None:
    if os.environ["GITHUB_REPOSITORY"] != "mavlink/qgroundcontrol":
        raise ValueError("Only upstream can publish qgc-dev")
    tag, source = os.environ["IMAGE_TAG"], os.environ["SOURCE_REVISION"]
    if tag == "latest":
        subprocess.run(["git", "fetch", "origin", "master"], check=True)
        subprocess.run(["git", "merge-base", "--is-ancestor", source, "origin/master"], check=True)
        changed = command("git", "diff", "--name-only", source, "origin/master").splitlines()
        if meaningful(changed):
            print("A newer meaningful image input supersedes this latest build")
            return
    elif release_source(tag) != source:
        raise ValueError("Release tag moved after image validation")
    platforms = {row["platform"] for row in json.loads(os.environ["IMAGE_MATRIX"])["include"]}
    if platforms != set(PLATFORMS):
        raise ValueError("Incomplete native platform matrix")
    digests = collect_digests(Path("digests"), platforms, source)
    registry = Registry()
    for platform, digest in digests.items():
        registry.verify_image(digest, platform, source)
    if tag != "latest":
        try:
            existing, digest = registry.read(tag)
        except urllib.error.HTTPError as error:
            if error.code != 404:
                raise
        else:
            registry.verify_index(existing, platforms, source)
            # A retry never replaces a valid stable tag, even if apt has changed.
            record_summary(tag, digest)
            return
    subprocess.run(
        [
            "docker",
            "buildx",
            "imagetools",
            "create",
            "--tag",
            f"{IMAGE}:{tag}",
            "--annotation",
            f"index:org.opencontainers.image.revision={source}",
            *[f"{IMAGE}@{digests[p]}" for p in sorted(digests)],
        ],
        check=True,
    )
    index, digest = registry.read(tag)
    registry.verify_index(index, platforms, source)
    record_summary(tag, digest)


def record_summary(tag: str, digest: str) -> None:
    with Path(os.environ["GITHUB_STEP_SUMMARY"]).open("a") as stream:
        stream.write(f"Image: `{IMAGE}:{tag}`\n\nDigest pin: `{IMAGE}@{digest}`\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("plan", "smoke", "publish"))
    {"plan": plan, "smoke": smoke, "publish": publish}[parser.parse_args().command]()


if __name__ == "__main__":
    main()
