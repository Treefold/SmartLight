import requests as req

INIT = req.post('http://localhost:9080/init/1')
print('INIT:', INIT.status_code, INIT.text)

GET_RGB_1 = req.get('http://localhost:9080/rgb/1')
print('GET_RGB_1:', GET_RGB_1.status_code, GET_RGB_1.text)

POST_RGB_1 = req.post('http://localhost:9080/rgb/1/420/100/0')
print('POST_RGB_1:', POST_RGB_1.status_code, POST_RGB_1.text)

GET_RGB_2 = req.get('http://localhost:9080/rgb/1')
print('GET_RGB_2:', GET_RGB_2.status_code, GET_RGB_2.text)

POST_RGB_2 = req.post('http://localhost:9080/rgb/1/42/100/0')
print('POST_RGB_2:', POST_RGB_2.status_code, POST_RGB_2.text)

GET_RGB_3 = req.get('http://localhost:9080/rgb/1')
print('GET_RGB_3:', GET_RGB_3.status_code, GET_RGB_3.text)