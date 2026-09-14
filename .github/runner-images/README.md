# RunsOn Runner Images

The `Runner Images` workflow builds `qgc-runs-on-ubuntu24-x64-*` from the current RunsOn Ubuntu 24
full image. The derived AMI preinstalls QGroundControl's Debian build dependencies and desktop Qt
SDK. RunsOn selects the newest matching AMI from the AWS account and region that host the RunsOn
stack.

Configure these repository settings before dispatching the workflow:

- Variable `RUNS_ON_AMI_SUBNET_ID`: public subnet used by the temporary Packer instance.
- Variable `AWS_REGION`: optional; defaults to `us-west-2`. It must match the RunsOn stack's region.
- Secret `RUNS_ON_AMI_ROLE_ARN`: OIDC role with the EC2 permissions Packer needs to build and
  register an AMI. The role must create the AMI in the RunsOn stack's AWS account and region.

Roll out the image in this order:

1. Configure the settings above.
2. Dispatch `Runner Images` from the repository's default branch.
3. Wait for both the AMI build and `Smoke Test Ubuntu 24 x64` jobs to pass. The smoke job boots the
   newly selected managed image and verifies the preinstalled Qt SDK and core Linux build tools.
4. Set `RUNS_ON_LINUX_BUILDER` to `linux-x64-builder-prebaked`.

Leave `RUNS_ON_LINUX_BUILDER` unset to continue using the standard RunsOn image. Rebuild at least
every 30 days (RunsOn recommends about every 15 days) so the base image, security updates, and
GitHub runner agent remain current. Image builds remain manual until the AWS account has an agreed
retention policy. The workflow does not delete older AMIs or EBS snapshots; retire those according
to that policy.

Windows warm pools are organization-level RunsOn configuration. Copy and review
`windows-warm-pool.example.yml` in the organization's `.github-private` repository, then set this
repository's `RUNS_ON_WINDOWS_POOL` variable to `qgc-windows-x64-builder`. Leave the variable unset
until the pool exists; workflows will continue using the ordinary `windows-x64-builder` runner.
