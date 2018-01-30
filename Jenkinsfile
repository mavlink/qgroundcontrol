pipeline {
  agent any
  stages {
    stage('build') {
      parallel {
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
            sh 'cd ${SHADOW_BUILD_DIR}; ~/Qt/5.9.3/clang_64/bin/qmake -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=release CONFIG+=WarningsAsErrorsOn'
            sh 'cd ${SHADOW_BUILD_DIR}; make -j24'
          }
        }
      }
    }
  }
  environment {
    SHADOW_BUILD_DIR = '/tmp/jenkins/shadow_build_dir'
  }
}