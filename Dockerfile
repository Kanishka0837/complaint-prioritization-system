FROM nginx:alpine

COPY ui.html /usr/share/nginx/html/index.html
COPY data.json /usr/share/nginx/html/data.json

EXPOSE 80
