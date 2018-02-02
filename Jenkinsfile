pipeline {
  agent any
  stages {
    stage('build') {
      parallel {
        stage('Linux Release') {
          agent {
            docker {
              image 'mavlink/qgc-build-linux'
              args '-e CI=true -e CCACHE_BASEDIR=$WORKSPACE -e CCACHE_DIR=/tmp/ccache -v /tmp/ccache:/tmp/ccache:rw'
            }
          }
          steps {
            sh 'git submodule deinit -f .'
            sh 'git clean -ff -x -d .'
            sh 'git submodule update --init --recursive --force'
            sh 'mkdir build; cd build; qmake -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=release CONFIG+=WarningsAsErrorsOn'
            sh 'cd build; make -j4'
            sh 'ccache -s'
          }
        }
        stage('OSX Debug') {
          agent {
            node {
              label 'mac'
            }
          }
          environment {
            QT_FATAL_WARNINGS = '1'
            QMAKESPEC = 'macx-clang'
          }
          steps {
            sh 'git submodule deinit -f .'
            sh 'git clean -ff -x -d .'
            sh 'git submodule update --init --recursive --force'
            sh 'rm -rf ${SHADOW_BUILD_DIR}; mkdir -p ${SHADOW_BUILD_DIR}'
            sh 'cd ${SHADOW_BUILD_DIR}; ${QT_PATH}/5.9.3/clang_64/bin/qmake -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=debug CONFIG+=WarningsAsErrorsOn'
            sh 'cd ${SHADOW_BUILD_DIR}; make -j`sysctl -n hw.ncpu`'
            sh 'ccache -s'
          }
        }
        stage('OSX Release') {
          agent {
            node {
              label 'mac'
            }
          }
          environment {
            QT_FATAL_WARNINGS = '1'
            QMAKESPEC = 'macx-clang'
          }
          steps {
            sh 'git submodule deinit -f .'
            sh 'git clean -ff -x -d .'
            sh 'git submodule update --init --recursive --force'
            sh 'rm -rf ${SHADOW_BUILD_DIR}; mkdir -p ${SHADOW_BUILD_DIR}'
            sh 'cd ${SHADOW_BUILD_DIR}; ${QT_PATH}/5.9.3/clang_64/bin/qmake -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=release CONFIG+=WarningsAsErrorsOn'
            sh 'cd ${SHADOW_BUILD_DIR}; make -j`sysctl -n hw.ncpu`'
            sh 'ccache -s'
          }
        }
      }
    }
  }
  environment {
    SHADOW_BUILD_DIR = '/tmp/jenkins/shadow_build_dir'
    CCACHE_CPP2 = '1'
    CCACHE_BASEDIR = '${WORKSPACE}'
  }
}
