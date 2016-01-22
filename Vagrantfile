# -*- mode: ruby -*-
# vi: set ft=ruby :

require 'yaml'

current_dir    = File.dirname(File.expand_path(__FILE__))
configfile        = YAML.load_file("#{current_dir}/.vagrantconfig.yml")
yaml_config = configfile['configs']['dev']

Vagrant.configure(2) do |config|
  config.vm.box = "ubuntu/trusty64"
  config.vm.provider :virtualbox do |vb|
    vb.customize ["modifyvm", :id, "--memory", "4096"]
    vb.customize ["modifyvm", :id, "--cpus", "1"]
  end

  # the "dev configuration puts the build products and a suitable
  # environment into the /vagrant directory.  This allows you to run
  # qgroundcontrol on the host machine with:
  # "cd shadow-build/release; ../../deploy/qgroundcontrol-start.sh"

  $config_shell = <<-'SHELL'
     sudo apt-get update -y
     sudo apt-get dist-upgrade -y
     sudo apt-get install -y git build-essential
     sudo apt-get install -y espeak libespeak-dev libudev-dev libsdl1.2-dev
     sudo apt-get install -y doxygen 
     sudo apt-get install -y gstreamer1.0* libgstreamer1.0*

     # taken from travis.yml
     su - vagrant -c 'wget --continue -q %{deps_url}/%{qt_deps_tarball}'
     su - vagrant -c 'rm -rf %{qt_deps_unpack_dir}'
     su - vagrant -c 'mkdir -p %{qt_deps_unpack_parent_dir}'
     su - vagrant -c 'tar jxf "%{qt_deps_tarball}" -C  %{qt_deps_unpack_parent_dir}'
     su - vagrant -c 'rm -rf %{shadow_build_dir}'
     su - vagrant -c 'mkdir -p %{shadow_build_dir}'
     su - vagrant -c "cd %{shadow_build_dir}; LD_LIBRARY_PATH=%{qt_deps_lib_unpack_dir} PATH=%{qt_deps_bin_unpack_dir}:\$PATH qmake -r %{pro} -spec %{spec}"
     su - vagrant -c "cd %{shadow_build_dir}; LD_LIBRARY_PATH=%{qt_deps_lib_unpack_dir} PATH=%{qt_deps_bin_unpack_dir}:\$PATH make -j4"

     su - vagrant -c 'mkdir -p %{qt_deps_dir}'
     su - vagrant -c 'cp -a %{qt_deps_bin_unpack_dir} %{qt_deps_bin_dir}'
     su - vagrant -c 'cp -a %{qt_deps_lib_unpack_dir} %{qt_deps_lib_dir}'
     su - vagrant -c 'cp -a %{qt_deps_plugins_unpack_dir} %{qt_deps_plugins_dir}'
     su - vagrant -c 'cp -a %{qt_deps_qml_unpack_dir} %{qt_deps_qml_dir}'

   SHELL

  config.vm.provision "dev", type: "shell", inline: $config_shell  % {
    :shadow_build_dir => yaml_config['shadow_build_dir'],
    :qt_deps_tarball => yaml_config['qt_deps_tarball'],
    :pro => yaml_config['pro'],
    :spec => yaml_config['spec'],
    :deps_url => yaml_config['deps_url'],

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
