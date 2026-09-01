packer {
  required_plugins {
    amazon = {
      source  = "github.com/hashicorp/amazon"
      version = "= 1.8.2"
    }
  }
}

variable "aws_region" {
  type = string
}

variable "subnet_id" {
  type = string
}

variable "source_repository" {
  type = string
}

variable "source_ref" {
  type = string
}

variable "qt_version" {
  type = string
}

variable "qt_modules" {
  type = string
}

source "amazon-ebs" "qgc_ubuntu24_x64" {
  ami_name                    = "qgc-runs-on-ubuntu24-x64-${formatdate("YYYYMMDD-hhmmss", timestamp())}"
  ami_description             = "QGroundControl Ubuntu 24 x64 CI image from ${var.source_ref}"
  associate_public_ip_address = true
  instance_type               = "c8i.xlarge"
  region                      = var.aws_region
  ssh_interface               = "public_ip"
  ssh_timeout                 = "10m"
  ssh_username                = "ubuntu"
  subnet_id                   = var.subnet_id
  user_data_file              = "${path.root}/user-data.sh"

  source_ami_filter {
    filters = {
      name                = "runs-on-v2.2-ubuntu24-full-x64-*"
      root-device-type    = "ebs"
      virtualization-type = "hvm"
    }
    most_recent = true
    owners      = ["135269210855"]
  }

  launch_block_device_mappings {
    device_name           = "/dev/sda1"
    delete_on_termination = true
    volume_size           = 100
    volume_type           = "gp3"
  }

  tags = {
    ManagedBy    = "packer"
    Project      = "qgroundcontrol"
    SourceCommit = var.source_ref
  }
}

build {
  sources = ["source.amazon-ebs.qgc_ubuntu24_x64"]

  provisioner "shell" {
    environment_vars = [
      "QGC_SOURCE_REPOSITORY=${var.source_repository}",
      "QGC_SOURCE_REF=${var.source_ref}",
      "QGC_QT_VERSION=${var.qt_version}",
      "QGC_QT_MODULES=${var.qt_modules}",
    ]
    script = "${path.root}/provision-linux.sh"
  }

  post-processor "manifest" {
    output = "packer-manifest.json"
  }
}
