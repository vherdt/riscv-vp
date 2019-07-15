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
            steps {
                archiveArtifacts artifacts: 'vp/build/bin/*'
                archiveArtifacts artifacts: 'env/basic/vp-display/build/vp-display'
                archiveArtifacts artifacts: 'env/hifive/vp-breadboard/build/vp-breadboard'
                //archiveArtifacts artifacts: 'env/riscview/vp-display/build/vp-display'
            }
        }
    }
    post {  
        always {  
            echo 'This will always run'
            mail    bcc: '', 
                    body: "<b>Build failed in Project ${env.JOB_NAME} - ${env.BRANCH_NAME}</b> (see ${env.BUILD_URL})</br>${GIT_COMMIT}", 
                    cc: '', 
                    charset: 'UTF-8', 
                    from: 'jenkins@informatik.uni-bremen.de', 
                    mimeType: 'text/html',
                    replyTo: 'plsdontask-ppieper@tzi.de',
                    subject: "Build failed in Jenkins: ${env.JOB_NAME} - ${env.BRANCH_NAME} - ${env.BUILD_NUMBER}",
                    to: "ppieper@informatik.uni-bremen.de, ${env.GIT_COMMITTER_EMAIL}";
            emailext attachLog: true
        }  
        success {  
            echo 'This will run only if successful'  
        }  
        failure {  
            mail    bcc: '', 
                    body: "<b>Build failed in Project ${env.JOB_NAME} - ${env.BRANCH_NAME}</b> (see ${env.BUILD_URL})</br>${GIT_COMMIT}", 
                    cc: '', 
                    charset: 'UTF-8', 
                    from: 'jenkins@informatik.uni-bremen.de', 
                    mimeType: 'text/html',
                    replyTo: 'plsdontask-ppieper@tzi.de',
                    subject: "Build failed in Jenkins: ${env.JOB_NAME} - ${env.BRANCH_NAME} - ${env.BUILD_NUMBER}",
                    to: "ppieper@informatik.uni-bremen.de, ${env.GIT_COMMITTER_EMAIL}";
            emailext attachLog: true
        }  
        unstable {  
            echo 'This will run only if the run was marked as unstable'  
        }  
        changed {  
            echo 'This will run only if the state of the Pipeline has changed'  
            echo 'For example, if the Pipeline was previously failing but is now successful'  
        }  
    } 
}