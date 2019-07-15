pipeline {
    agent {
        label: "fedora-28 || ubuntu-18.04 || debian-9"
    }
//    environment {
//        CI = 'true'
//    }
    stages {
        stage('Build') {
            steps {
                sh 'make all'
            }
        }
        stage('Test') {
//            steps {
//                sh './jenkins/scripts/test.sh'
//            }
        }
    }
}