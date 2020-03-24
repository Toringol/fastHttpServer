# fastHttpServer

# RUN

docker build -t shepelev-httpd https://github.com/Toringol/fastHttpServer.git  
docker run -p 80:80 -v /etc/httpd.conf:/etc/httpd.conf:ro -v /var/www/html:/var/www/html:ro --name shepelev-httpd -t shepelev-httpd  

# Load

ab -n 100000 -c 100 localhost/httptest/wikipedia_russia.html  
