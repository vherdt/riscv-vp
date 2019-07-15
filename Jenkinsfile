pipeline {
    agent {
        label "fedora-28 || ubuntu-18.04 || debian-9"
    }
    stages {
        stage('Build') {
            steps {
                sh 'make all'
            }
        }
//        stage('Test') {
//            steps {
//                sh './jenkins/scripts/test.sh'
//            }
//        }
        stage ("Archive") {
              archiveArtifacts artifacts: 'vp/build/bin/*'
              archiveArtifacts artifacts: 'env/basic/vp-display/build/vp-display'
              archiveArtifacts artifacts: 'env/hifive/vp-breadboard/build/vp-breadboard'
              //archiveArtifacts artifacts: 'env/riscview/vp-display/build/vp-display'
        }
    }
}