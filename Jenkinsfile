pipeline {
    agent {
        label "fedora-28 || ubuntu-18.04 || debian-9"
    }
    environment {
        DEVELOPERS = "vherdt@informatik.uni-bremen.de ppieper@informatik.uni-bremen.de nbruns@informatik.uni-bremen.de tempel@informatik.uni-bremen.de"
    
        GIT_COMMIT_MSG = sh (script: 'git log -n1 --pretty=format:"%s"', returnStdout: true).trim()
        GIT_COMMIT_TIM = sh (script: 'git log -n1 --pretty=format:"%ai"', returnStdout: true).trim()
        GIT_COMMITTER  = sh (script: 'git log -n1 --pretty=format:"%an"', returnStdout: true).trim()
        GIT_COMMITTER_MAIL  = sh (script: 'git log -n1 --pretty=format:"%ae"', returnStdout: true).trim()
    }
    stages {
        stage('Build') {
            steps {
                //sh 'make all'
                sh 'echo mock-build'
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

            
        }  
        success {  
            echo 'This will run only if successful'  
        }  
        failure {  
            echo 'This will run only if not successful'  
            emailext(
                //recipientProviders: [culprits, brokenBuildSuspects],
                attachLog: true,
                body:
                """<b>${env.GIT_COMMITTER} broke Project ${env.JOB_NAME} ${env.BUILD_NUMBER}</b></br>
                ${env.GIT_COMMIT_MSG}
                </br>(ask ${env.GIT_COMMITTER_MAIL} or see ${env.BUILD_URL})
                """,
                from: 'jenkins@informatik.uni-bremen.de', 
                mimeType: 'text/html',
                replyTo: 'plsdontask-ppieper@tzi.de',
                subject: "Build failed in Jenkins: ${env.JOB_NAME}",
                to: "${env.DEVELOPERS}"
            )
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