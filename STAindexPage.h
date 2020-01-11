/*
Это не макрос, это просто часть языка (представленная в стандарте C ++ 11), известная как необработанный строковый 
литерал.
Сырые строковые литералы выглядят как
R «маркер (текст) маркер»
последовательность ') маркер' должен быть выбран таким образом, чтобы он не появляется в тексте . 
В приведенном вами примере этот токен состоит из пяти знаков равенства
*/
const char STAIndexPage[] PROGMEM = R"=====(
<html>
<head>
<meta charset="utf-8">
<title>3DStol</title>
</head>
<body>
<center>Режим STA</center>

<form action="/urlSaveCfgCitiidApiKey" method="GET">
<br/>Код города
<br/><input type="text" name="citiID" style="width:400px;">
<br/>Ваш APIKEY
<br/><input type="text" name="apiKey" style="width:400px;">
<br/>
<br/><input type="submit" value="Сохранить">
</form>
<br/>
Cброс настроек
<a href="/urlWriteDefaultApiKey">Сбросить</a>

<body>
</html>
)=====";
