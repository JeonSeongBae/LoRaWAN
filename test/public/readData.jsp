<%@page import="java.io.IOException"%>
<%@ page language="java" contentType="text/html; charset=UTF-8"
	pageEncoding="UTF-8"%>

<%@page import="java.io.*"%>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<title>checkLogin_로그인 확인</title>
</head>
<body>
	<%
		// 정보를 받아옴
		String data = request.getParameter("filed1");

		// 응답을 전해줄 객체를 생성하여 전해줌
		PrintWriter printWriter = response.getWriter();
		printWriter.print(data);
		printWriter.flush();
		printWriter.close();
	%>
</body>
</html>
