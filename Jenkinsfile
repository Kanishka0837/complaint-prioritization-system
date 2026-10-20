pipeline {
    agent any

    environment {
        AWS_REGION = 'ap-south-1'
        ECR_REPOSITORY = '905063831150.dkr.ecr.ap-south-1.amazonaws.com/complaint-prioritization-system'
        IMAGE_TAG = 'latest'
    }

    stages {
        stage('Build and Test C++') {
            steps {
                sh '''
                    clang++ -std=c++17 -Wall -Wextra main.cpp -o complaint-system
                    test -f ui.html
                    python3 -m json.tool data.json > /dev/null
                '''
            }
        }

        stage('Build Docker Image') {
            steps {
                sh 'docker build --no-cache -t $ECR_REPOSITORY:$IMAGE_TAG .'
            }
        }

        stage('Push Image to AWS ECR') {
            steps {
                sh '''
                    aws ecr get-login-password --region $AWS_REGION --profile cca2 |
                    docker login --username AWS --password-stdin 905063831150.dkr.ecr.ap-south-1.amazonaws.com
                    docker push $ECR_REPOSITORY:$IMAGE_TAG
                '''
            }
        }

        stage('Deploy to Minikube') {
            steps {
                sh '''
                    kubectl apply -f k8s/deployment.yaml
                    kubectl apply -f k8s/service.yaml
                    kubectl rollout restart deployment/complaint-prioritization
                    kubectl rollout status deployment/complaint-prioritization --timeout=180s
                '''
            }
        }
    }
}
