variable "SOURCE_REVISION" {
  default = ""
}

group "default" {
  targets = ["qgc-dev"]
}

target "qgc-dev" {
  context = "."
  dockerfile = "deploy/docker/Dockerfile"
  target = "qgc-dev"
  platforms = ["linux/amd64", "linux/arm64"]
  tags = ["qgc-dev:local"]
  labels = {
    "org.opencontainers.image.source" = "https://github.com/mavlink/qgroundcontrol"
    "org.opencontainers.image.revision" = SOURCE_REVISION
  }
}
