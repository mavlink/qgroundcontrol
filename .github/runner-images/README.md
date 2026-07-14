# RunsOn Runner Images

`runner-images.yml` manually builds `qgc-runs-on-ubuntu24-x64-*` from the current RunsOn Ubuntu 24
full image. The derived image preinstalls QGC's Debian packages and desktop Qt SDK.

Configure these repository settings before running the workflow:

- Variable `RUNS_ON_AMI_SUBNET_ID`: subnet used by the temporary Packer instance.
- Variable `AWS_REGION`: optional; defaults to `us-west-2`.
- Secret `RUNS_ON_AMI_ROLE_ARN`: OIDC role with EC2 permissions to build and register an AMI.
  `AWS_ROLE_ARN` is used as a fallback. The role must build in the RunsOn stack's AWS account so
  the account-local image lookup can find the result.

After the first AMI succeeds, set `RUNS_ON_LINUX_BUILDER=linux-x64-builder-prebaked`. Leave the
variable unset to keep using the standard RunsOn image. Run the workflow at least monthly so the
base image and security updates remain current.

Windows warm pools must be configured in the organization's private RunsOn repository. Copy and
review `windows-warm-pool.example.yml` there, then set this repository's
`RUNS_ON_WINDOWS_POOL=qgc-windows-x64-builder`. Leave the variable unset until the pool exists;
workflows will continue using the ordinary `windows-x64-builder` runner.
