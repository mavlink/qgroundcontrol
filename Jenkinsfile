pipeline {
  agent none
  stages {

    stage('build') {
      parallel {

        // stage('Android Release') {
        //   environment {
        //     CCACHE_BASEDIR = "${env.WORKSPACE}"
        //     QGC_CONFIG = 'installer'
        //     QMAKE_VER = "5.11.0/android_armv7/bin/qmake"
        //   }
        //   agent {
        //     label 'docker'
        //     /*
        //     docker {
        //       label 'docker'
        //       image 'mavlink/qgc-build-android:2019-02-03'
        //       args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
        //     }
        //     */
        //     dockerfile {
        //       dir 'custom/deploy/ci/android'
        //       args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
        //     }
        //   }
        //   steps {
        //     sh 'export'
        //     sh 'ccache -z'
        //     sh 'git submodule deinit -f .'
        //     sh 'git clean -ff -x -d .'
        //     sh 'git submodule update --init --recursive --force'
        //     sh 'ln -s $CI_ANDROID_GSTREAMER_LOCATION ${WORKSPACE}/'
        //     sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
        //     sh 'cd build; make -j`nproc --all`'
        //     sh 'ccache -s'
        //     sh 'cp build/release/package/*.apk ${WORKSPACE}/'
        //   }
        //   post {
        //     always {
        //       archiveArtifacts artifacts: 'build/release/**/*'
        //       archiveArtifacts artifacts: '*.apk', onlyIfSuccessful: true
        //     }
        //     cleanup {
        //       sh 'git clean -ff -x -d .'
        //     }
        //   }
        // }

        /*
        stage('Linux Debug') {
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            QGC_CONFIG = 'debug'
            QMAKE_VER = "5.11.0/gcc_64/bin/qmake"
          }
          agent {
            docker {
              image 'mavlink/qgc-build-linux:2019-02-03'
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

        stage('Linux Debug (cmake)') {
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            CMAKE_BUILD_TYPE = 'Debug'
            QT_VERSION = "5.11.0"
            QT_MKSPEC = "gcc_64"
          }
          agent {
            docker {
              image 'mavlink/qgc-build-linux:2019-02-03'
              args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
            }
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'make distclean'
            sh 'make submodulesclean'
            sh 'make linux'
            //sh 'make linux check' // TODO: needs Xvfb or similar
            sh 'ccache -s'
            sh 'make distclean'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }
        */

        // stage('Linux Release') {
        //   environment {
        //     CCACHE_BASEDIR = "${env.WORKSPACE}"
        //     QGC_CONFIG = 'release'
        //     QMAKE_VER = "5.11.0/gcc_64/bin/qmake"

        //     QGC_CUSTOM_APP_NAME = "AGS"
        //     QGC_CUSTOM_GENERIC_NAME = "Auterion Ground Station"
        //     QGC_CUSTOM_BINARY_NAME = "AuterionGS"
        //     QGC_CUSTOM_LINUX_START_SH = "${env.WORKSPACE}/custom/deploy/qgroundcontrol-start.sh"
        //     QGC_CUSTOM_APP_ICON = "${env.WORKSPACE}/custom/res/src/Auterion_Icon.png"
        //     QGC_CUSTOM_APP_ICON_NAME = "Auterion_Icon"
        //   }
        //   agent {
        //       label 'docker'
        //     /*docker {
        //       image 'mavlink/qgc-build-linux:2019-02-03'
        //       args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw --privileged --cap-add SYS_ADMIN --device /dev/fuse'
        //     }*/
        //     // TODO: remove after sync upstream
        //     // Custom docker file that extends on the upstream one in order to provide unmerged features
        //     dockerfile {
        //       dir 'custom/deploy/ci/linux'
        //       args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw --privileged --cap-add SYS_ADMIN --device /dev/fuse'
        //     }
        //   }
        //   steps {
        //     sh 'export'
        //     sh 'ccache -z'
        //     sh 'git submodule deinit -f .'
        //     sh 'git clean -ff -x -d .'
        //     sh 'git submodule update --init --recursive --force'
        //     sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
        //     sh 'cd build; make -j`nproc --all`'

        //     // Create AppImg
        //     sh 'deploy/create_linux_appimage.sh ${WORKSPACE}/ ${WORKSPACE}/build/release/'
        //     sh 'chmod +x AuterionGS.AppImage'

        //     // Cache build files
        //     sh 'ccache -s'
        //   }
        //   post {
        //     always {
        //         archiveArtifacts artifacts: 'build/release/**/*', onlyIfSuccessful: true
        //         archiveArtifacts artifacts: 'AuterionGS.AppImage', onlyIfSuccessful: true
        //     }
        //     cleanup {
        //       sh 'git clean -ff -x -d .'
        //     }
        //   }
        // }

        /*
        stage('Linux Release (cmake)') {
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            CMAKE_BUILD_TYPE = 'Release'
            QT_VERSION = "5.11.0"
            QT_MKSPEC = "gcc_64"
          }
          agent {
            docker {
              image 'mavlink/qgc-build-linux:2019-02-03'
              args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
            }
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'make distclean'
            sh 'make submodulesclean'
            sh 'make linux'
            sh 'ccache -s'
            sh 'make distclean'
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

        stage('OSX Debug (cmake)') {
          agent {
            node {
              label 'mac'
            }
          }
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            CMAKE_BUILD_TYPE = 'Debug'
            QT_VERSION = "5.11.0"
            QT_MKSPEC = "clang_64"
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'make distclean'
            sh 'make submodulesclean'
            sh 'make mac'
            sh 'ccache -s'
            sh 'make distclean'
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
                */
                //archiveArtifacts(artifacts: 'build/**/*.dmg', fingerprint: true)
                /*
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

        stage('OSX Release (cmake)') {
          agent {
            node {
              label 'mac'
            }
          }
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            CMAKE_BUILD_TYPE = 'Release'
            QT_VERSION = "5.11.0"
            QT_MKSPEC = "clang_64"
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'make distclean'
            sh 'make submodulesclean'
            sh 'make mac'
            sh 'ccache -s'
            sh 'make distclean'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }
        */

        stage('Windows Release') {
          environment {
            QGC_CONFIG = 'release installer separate_debug_info force_debug_info qtquickcompiler'
          }
          agent {
            node {
              label 'windows'
            }
          }
          steps {
            bat 'git submodule deinit -f .'
            bat 'git clean -ff -x -d .'
            bat 'git submodule update --init --recursive --force'
            bat '.\\tools\\build\\build_windows.bat'
          }
          post {
            always {
                archiveArtifacts artifacts: 'build/release/**/*', onlyIfSuccessful: true
                archiveArtifacts artifacts: '*-installer.exe', onlyIfSuccessful: true
            }
            cleanup {
              bat 'git clean -ff -x -d .'
            }
          }
        }

        stage('Dev Windows Release (Update)') {
          environment {
            QGC_CONFIG = 'release installer separate_debug_info force_debug_info qtquickcompiler'
          }
          agent {
            node {
              label 'windows'
            }
          }
          steps {
            bat 'if exist .\\build\\release\\*.exe (del /F /Q .\\build\\release\\*.exe)'
            bat 'if exist .\\*-installer.exe (del /F /Q .\\*-installer.exe)'
            bat '.\\tools\\build\\build_windows.bat'
          }
          post {
            always {
                archiveArtifacts artifacts: 'build/release/**/*', onlyIfSuccessful: true
                archiveArtifacts artifacts: '*-installer.exe', onlyIfSuccessful: true
            }
            cleanup {
              bat "echo Don't cleanup, we reuse the build. Not safe though"
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
    buildDiscarder(logRotator(artifactNumToKeepStr: '10', artifactDaysToKeepStr: '30', numToKeepStr: '30', daysToKeepStr: '90'))
    timeout(time: 60, unit: 'MINUTES')
  }
}
