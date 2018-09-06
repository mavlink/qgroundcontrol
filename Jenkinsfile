pipeline {
  agent none
  stages {

    stage('build') {
      parallel {

        stage('Android Release') {
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            QGC_CONFIG = 'release'
            QMAKE_VER = "5.11.0/android_armv7/bin/qmake"
          }
          agent {
            docker {
              image 'mavlink/qgc-build-android:2018-06-08'
              args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
            }
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'git submodule deinit -f .'
            sh 'git clean -ff -x -d .'
            sh 'git submodule update --init --recursive --force'
            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
            sh 'cd build; make -j`nproc --all`'
            sh 'ccache -s'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }

        stage('Linux Debug') {
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            QGC_CONFIG = 'debug'
            QMAKE_VER = "5.11.0/gcc_64/bin/qmake"
          }
          agent {
            docker {
              image 'mavlink/qgc-build-linux:2018-06-08'
              args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
            }
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'git submodule deinit -f .'
            sh 'git clean -ff -x -d .'
            sh 'git submodule update --init --recursive --force'
            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
            sh 'cd build; make -j`nproc --all`'
            sh 'ccache -s'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }

        stage('Linux Release') {
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            QGC_CONFIG = 'release'
            QMAKE_VER = "5.11.0/gcc_64/bin/qmake"
          }
          agent {
            docker {
              image 'mavlink/qgc-build-linux:2018-06-08'
              args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
            }
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'git submodule deinit -f .'
            sh 'git clean -ff -x -d .'
            sh 'git submodule update --init --recursive --force'
            withCredentials([file(credentialsId: 'QGC_Airmap_api_key', variable: 'AIRMAP_API_HEADER')]) {
              sh 'cp $AIRMAP_API_HEADER ${WORKSPACE}/src/Airmap/Airmap_api_key.h'
            }
            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
            sh 'cd build; make -j`nproc --all`'
            sh 'ccache -s'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }

        stage('OSX Debug') {
          agent {
            node {
              label 'mac'
            }
          }
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            QGC_CONFIG = 'debug'
            QMAKE_VER = "5.11.0/clang_64/bin/qmake"
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'git submodule deinit -f .'
            sh 'git clean -ff -x -d .'
            sh 'git submodule update --init --recursive --force'
            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
            sh 'cd build; make -j`sysctl -n hw.ncpu`'
            sh 'ccache -s'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }

        stage('OSX Release') {
          agent {
            node {
              label 'mac'
            }
          }
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            QGC_CONFIG = 'installer'
            QMAKE_VER = "5.11.0/clang_64/bin/qmake"
          }
          stages {
            stage('Clean Checkout') {
              steps {
                sh 'export'
                sh 'ccache -z'
                sh 'git submodule deinit -f .'
                sh 'git clean -ff -x -d .'
                sh 'git submodule update --init --recursive --force'
              }
            }

            stage('Add Airmap API key') {
              steps {
                withCredentials([file(credentialsId: 'QGC_Airmap_api_key', variable: 'AIRMAP_API_HEADER')]) {
                  sh 'cp $AIRMAP_API_HEADER ${WORKSPACE}/src/Airmap/Airmap_api_key.h'
                }
              }
              when {
                anyOf {
                  branch 'master';
                  branch 'Stable_*'
                }
              }
            }

            stage('Build OSX Release') {
              steps {
                sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
                sh 'cd build; make -j`sysctl -n hw.ncpu`'
                archiveArtifacts(artifacts: 'build/**/*.dmg', fingerprint: true)
                sh 'ccache -s'
              }
            }
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }

      } // parallel
    } // stage('build')
  } // stages

  environment {
    CCACHE_CPP2 = '1'
    CCACHE_DIR = '/tmp/ccache'
    QT_FATAL_WARNINGS = '1'
  }

  options {
    buildDiscarder(logRotator(numToKeepStr: '10', artifactDaysToKeepStr: '30'))
    timeout(time: 60, unit: 'MINUTES')
  }
}
