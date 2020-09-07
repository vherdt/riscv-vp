pipeline {
    agent {
        label "fedora-28 || ubuntu-18.04 || debian-latest"
    }
    environment {
        DEVELOPERS = "ppieper@informatik.uni-bremen.de vherdt@informatik.uni-bremen.de  nbruns@informatik.uni-bremen.de tempel@informatik.uni-bremen.de"
    
        GIT_COMMIT_MSG = sh (script: 'git log -n1 --pretty=format:"%s"', returnStdout: true).trim()
        GIT_COMMIT_TIM = sh (script: 'git log -n1 --pretty=format:"%ai"', returnStdout: true).trim()
        GIT_COMMITTER  = sh (script: 'git log -n1 --pretty=format:"%an"', returnStdout: true).trim()
        GIT_COMMITTER_MAIL = sh (script: 'git log -n1 --pretty=format:"%ae"', returnStdout: true).trim()

        PATH = "${env.WORKSPACE}/rv32/bin:${env.WORKSPACE}/rv64/bin:${env.PATH}"
        GDB_DEBUG_PROG = "riscv64-unknown-elf-gdb"
    }
    stages {
        stage('Get Artifacts'){
            steps{
                copyArtifacts(projectName: 'gnu-toolchain_riscv32', target: 'rv32', selector: specific("81"));
                sh """
                cd rv32
                tar xzf gnu-toolchain_riscv32* --strip-components=1
                """

                copyArtifacts(projectName: 'gnu-toolchain_riscv64', target: 'rv64', selector: specific("82"));
                sh """
                cd rv64
                tar xzf gnu-toolchain_riscv64* --strip-components=1
                """
            }
        }
        stage('Build') {
            steps {
                sh 'make all'
                //sh 'echo mock-build'
            }
        }
        stage('Test') {
            steps {
                sh 'cd vp/build && ctest -V'
            }
        }
        stage ("Archive") {
            steps {
                archiveArtifacts artifacts: 'vp/build/bin/*'
                archiveArtifacts artifacts: 'env/basic/vp-display/build/vp-display'
                archiveArtifacts artifacts: 'env/hifive/vp-breadboard/build/vp-breadboard'
            }
        }
    }
    post {  
        //always {  
        //    echo 'This will always run'
        //}  
        success {  
            //echo 'This will run only if successful'  
            script {
                if (currentBuild.result == null) {
                    currentBuild.result = 'SUCCESS'    
                }
            }
            script {
                if (currentBuild.previousBuild != null && currentBuild.previousBuild.result != 'SUCCESS') {
                    emailext(
                        //recipientProviders: [culprits, brokenBuildSuspects],
                        attachLog: false,
                        subject: "Build back to normal: ${env.JOB_NAME}",
                        from: 'jenkins@informatik.uni-bremen.de', 
                        mimeType: 'text/html',
                        to: "${env.DEVELOPERS}",
                        body:
                        """<b>${env.GIT_COMMITTER} repaired Project ${env.JOB_NAME} #${env.BUILD_NUMBER}</b></br>
                        <blockquote><code>
                            ${env.GIT_COMMIT_MSG}
                        </code></blockquote>
                        """
                    )
                } 
            }
        }  
        failure {  
            //echo 'This will run only if not successful'  
            emailext(
                //recipientProviders: [culprits, brokenBuildSuspects],
                attachLog: true,
                subject: "Build failed in Jenkins: ${env.JOB_NAME}",
                from: 'jenkins@informatik.uni-bremen.de', 
                mimeType: 'text/html',
                to: "${env.DEVELOPERS}",
                body:
                """<b>${env.GIT_COMMITTER} broke Project ${env.JOB_NAME} #${env.BUILD_NUMBER}</b></br>
                <blockquote><code>
                    ${env.GIT_COMMIT_MSG}
                </code></blockquote>
                </br>(ask ${env.GIT_COMMITTER_MAIL} or see <a href=${env.BUILD_URL}>this link, if you have the ssh-tunnel active</a>)
                """
            )
            script {
                if (currentBuild.result == null) {
                    currentBuild.result = 'FAILURE'
                }
            }
        }  
        unstable {  
            echo 'This will run only if the run was marked as unstable'  
        }  
        //changed {  
        //    echo 'This will run only if the state of the Pipeline has changed'  
        //    echo 'For example, if the Pipeline was previously failing but is now successful'  
        //}  
    } 
}
