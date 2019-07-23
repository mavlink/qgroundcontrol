pipeline {
	agent none
	stages {
		stage('build') {
			parallel {
				stage('Windows Release') {
					agent {
						node {
							label 'windows'
						}

					}
					//environment {
					//	CCACHE_BASEDIR = "${env.WORKSPACE}"
					//	QGC_CONFIG = 'release'
					//	QMAKE_VER = '5.11.0/gcc_64/bin/qmake'
					//}
					//post {
					//	cleanup {
					//		sh 'git clean -ff -x -d .'

					//	}

					//}
					steps {
						echo "Test"
						bat "call vcvarsall.bat"
						bat "call winbuild.bat"
						//sh '"mkdir %LOCALAPPDATA%/QtProject && copy test/qtlogging.ini %LOCALAPPDATA%/QtProject/"'
						//withCredentials(bindings: [file(credentialsId: 'QGC_Airmap_api_key', variable: 'AIRMAP_API_HEADER')]) {
						//	sh 'cp $AIRMAP_API_HEADER ${WORKSPACE}/src/Airmap/Airmap_api_key.h'
						//}

						//bat 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
						//bat 'cd build; jom`'
					}
				}
			}
		}
	}
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
