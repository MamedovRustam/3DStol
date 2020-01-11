/*
Это не макрос, это просто часть языка (представленная в стандарте C ++ 11), известная как необработанный строковый 
литерал.
Сырые строковые литералы выглядят как
R «маркер (текст) маркер»
последовательность ') маркер' должен быть выбран таким образом, чтобы он не появляется в тексте . 
В приведенном вами примере этот токен состоит из пяти знаков равенства
*/
const char APIndexPage[] PROGMEM = R"=====(
<html>
<head>
<meta charset="utf-8">
<title>3DStol</title>
</head>
<body>
<form action="/urlSaveCfgSsidPass" method="GET">
<br/>Ваш SSID
<br/><input type="text" name="loginHomeSsid" style="width:400px;">
<br/>Пороль
<br/><input type="text" name="passHomeSsid" style="width:400px;">
<br/>
<br/><input type="submit" value="Подключиться">
</form>
<body>
</html>
)=====";
