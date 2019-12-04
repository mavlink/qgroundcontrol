# -*- mode: ruby -*-
# vi: set ft=ruby :

# if you update this file, please consider updating .travis.yml too

require 'yaml'

current_dir    = File.dirname(File.expand_path(__FILE__))
configfile     = YAML.load_file("#{current_dir}/.vagrantconfig.yml")
travisfile     = YAML.load_file("#{current_dir}/.travis.yml")
yaml_config    = configfile['configs']['dev']

Vagrant.configure(2) do |config|
  # This trick is used to prefer a VM box over docker
  config.vm.provider "virtualbox"
  config.vm.provider "vmware_fusion"

  config.vm.box = "ubuntu/xenial64"
  config.vm.provider :docker do |docker, override|
    override.vm.box = "tknerr/baseimage-ubuntu-16.04"
  end
  config.vm.provider :virtualbox do |vb|
    vb.customize ["modifyvm", :id, "--memory", "6144"]
    vb.customize ["modifyvm", :id, "--cpus", "1"]
    vb.gui = true
  end
  ["vmware_fusion", "vmware_workstation"].each do |p|
    config.vm.provider p do |v|
      v.vmx["memsize"] = "6144"
      v.vmx["numvcpus"] = "1"
      v.gui = true
    end
  end
  if Vagrant.has_plugin?("vagrant-cachier")
    config.cache.scope = :box
    config.cache.synced_folder_opts = {
      owner: "_apt"
    }
  end

  # the "dev configuration puts the build products and a suitable
  # environment into the /vagrant directory.  This allows you to run
  # qgroundcontrol on the host machine with:
  # "cd shadow-build/release; ./qgroundcontrol-start.sh"

  $config_shell = <<-'SHELL'
     set -e
     set -x

     export %{build_env}
     export JOBS=$((`cat /proc/cpuinfo | grep -c ^processor`+1))

     sudo apt-get update -y
     # we need this long command to keep packages (grub-pc esp.) from prompting for input
     sudo DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" dist-upgrade
     sudo DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" install %{apt_pkgs} xubuntu-desktop qtcreator
     sudo systemctl set-default graphical.target

     echo 'Initialising submodules'
     su - vagrant -c 'cd %{project_root_dir}; git submodule init && git submodule update'

     echo 'Saving %{qt_deps_tarball} from %{deps_url} to %{project_root_dir}'
     su - vagrant -c 'wget --continue -q %{deps_url} -P %{project_root_dir}'
     su - vagrant -c 'rm -rf %{qt_deps_unpack_dir}'
     su - vagrant -c 'mkdir -p %{qt_deps_unpack_parent_dir}'
     su - vagrant -c 'cd %{project_root_dir}; tar jxf "%{qt_deps_tarball}" -C  %{qt_deps_unpack_parent_dir}'
     su - vagrant -c 'rm -rf %{shadow_build_dir}'

     su - vagrant -c 'mkdir -p %{shadow_build_dir}'

     # write out a pair of scripts to make rebuilding on the VM easy:
     su - vagrant -c "cat <<QMAKE >do-qmake.sh
#!/bin/bash

set -e
set -x

cd %{shadow_build_dir}
export LD_LIBRARY_PATH=%{qt_deps_lib_unpack_dir}
export PATH=%{qt_deps_bin_unpack_dir}:\$PATH
qmake -r %{pro} CONFIG+=\${CONFIG} CONFIG+=WarningsAsErrorsOn -spec %{spec}
QMAKE
"

     su - vagrant -c "cat <<MAKE >do-make.sh
#!/bin/bash

set -e
set -x

cd %{shadow_build_dir}
export LD_LIBRARY_PATH=%{qt_deps_lib_unpack_dir}
export PATH=%{qt_deps_bin_unpack_dir}:\$PATH
make -j${JOBS}
MAKE
"
    su - vagrant -c "chmod +x do-qmake.sh do-make.sh"

    # now run the scripts:
    su - vagrant -c ./do-qmake.sh
    su - vagrant -c ./do-make.sh

   SHELL

  config.vm.provision "dev", type: "shell", inline: $config_shell  % {
    :shadow_build_dir => yaml_config['shadow_build_dir'],
    :qt_deps_tarball => yaml_config['qt_deps_tarball'],
    :pro => yaml_config['pro'],
    :spec => yaml_config['spec'],
    :deps_url => yaml_config['deps_url'],
    :apt_pkgs => (travisfile['addons']['apt']['packages']+['git', 'build-essential', 'fuse', 'libsdl2-dev']).join(' '),
    :build_env => travisfile['env']['global'].select { |item| item.is_a?(String) }.join(' '),

    :project_root_dir => yaml_config['project_root_dir'],
    :qt_deps_unpack_parent_dir => yaml_config['qt_deps_unpack_parent_dir'],
    :qt_deps_unpack_dir => yaml_config['qt_deps_unpack_dir'],
    :qt_deps_bin_unpack_dir => yaml_config['qt_deps_bin_unpack_dir'],
    :qt_deps_lib_unpack_dir => yaml_config['qt_deps_lib_unpack_dir'],
    :qt_deps_plugins_unpack_dir => yaml_config['qt_deps_plugins_unpack_dir'],
    :qt_deps_qml_unpack_dir => yaml_config['qt_deps_qml_unpack_dir'],

    :qt_deps_dir => yaml_config['qt_deps_dir'],
    :qt_deps_bin_dir => yaml_config['qt_deps_bin_dir'],
    :qt_deps_lib_dir => yaml_config['qt_deps_lib_dir'],
    :qt_deps_plugins_dir => yaml_config['qt_deps_plugins_dir'],
    :qt_deps_qml_dir => yaml_config['qt_deps_qml_dir'],
  }


end
