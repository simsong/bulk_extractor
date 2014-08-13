// SmtpServer.java
//
// Usage:
//
//    java SmtpServer <IP address> [<port>]
//
// port is optional, default port is 25
// output is to console and to file msg.txt
//

import java.io.*;
import java.net.*;


public class SmtpServer extends Thread {

    InetAddress mAddress;
    Socket mSocket;


    public SmtpServer(Socket aSocket, InetAddress aAddress) {
        mSocket = aSocket;
        mAddress = aAddress;
    }


    public void run() {
        try {
            go();
        }
        catch (Exception e) {
            System.out.println(e);
        }
        try {
            mSocket.close();
        }
        catch (IOException e) {
            System.out.println(e);
        }
    }


    public void go() throws Exception {
        FileOutputStream outFileStrm = new FileOutputStream("msg.txt");
        PrintWriter outFile = new PrintWriter(outFileStrm);
        InputStream inStrm = mSocket.getInputStream();
        OutputStream outStrm = mSocket.getOutputStream();
        BufferedReader in = new BufferedReader(new InputStreamReader(inStrm));
        PrintWriter out = new PrintWriter(new OutputStreamWriter(outStrm));
        System.out.println("Connected to "+
            mSocket.getInetAddress().getHostAddress());
        System.out.println("S: 220 "+mAddress);
        outFile.println("S: 220 "+mAddress);
        out.println("220 "+mAddress);
        out.flush();
        loop:
        while (true) {
            String line = in.readLine();
            if (line.startsWith("HELO")
                || line.startsWith("helo")) {
                System.out.println("C: "+line);
                outFile.println("C: "+line);
                System.out.println("S: 250 OK");
                outFile.println("S: 250 OK");
                out.println("250 OK");
                out.flush();
            }
            else if (line.startsWith("MAIL FROM:")
                || line.startsWith("mail from:")) {
                System.out.println("C: "+line);
                outFile.println("C: "+line);
                System.out.println("S: 250 OK");
                outFile.println("S: 250 OK");
                out.println("250 OK");
                out.flush();
            }
            else if (line.startsWith("RCPT TO:")
                || line.startsWith("rcpt to:")) {
                System.out.println("C: "+line);
                outFile.println("C: "+line);
                System.out.println("S: 250 OK");
                outFile.println("S: 250 OK");
                out.println("250 OK");
                out.flush();
            }
            else if (line.startsWith("DATA")
                || line.startsWith("data")) {
                System.out.println("C: "+line);
                outFile.println("C: "+line);
                System.out.println("S: 354 End data with <CR><LF>.<CR><LF>");
                outFile.println("S: 354 End data with <CR><LF>.<CR><LF>");
                out.println("354 End data with <CR><LF>.<CR><LF>");
                out.flush();
                int lineCount = 0;
                while (true) {
                    line = in.readLine();
                    if (line.equals(".")) {
                        System.out.println("C: "+line);
                        outFile.println("C: "+line);
                        System.out.println("S: 250 OK");
                        outFile.println("S: 250 OK");
                        out.println("250 OK");
                        out.flush();
                        break;
                    }
                    else {
                        System.out.println("C: "+line);
                        outFile.println("C: "+line);
                    }
                    ++lineCount;
                }
            }
            else if (line.startsWith("QUIT")
                || line.startsWith("quit")) {
                System.out.println("C: "+line);
                outFile.println("C: "+line);
                System.out.println("S: 221 Bye");
                outFile.println("S: 221 Bye");
                out.println("221 Bye");
                out.flush();
                break loop;
            }
            else if (line.startsWith("EHLO")
                || line.startsWith("ehlo")) {
                System.out.println("C: "+line);
                outFile.println("C: "+line);
                System.out.println("S: 500 Unknown command");
                outFile.println("S: 500 Unknown command");
                out.println("500 Unknown command");
                out.flush();
            }
            else if (line.startsWith("RSET")
                || line.startsWith("rset")) {
                System.out.println("C: "+line);
                outFile.println("C: "+line);
                System.out.println("S: 250 OK");
                outFile.println("S: 250 OK");
                out.println("250 OK");
                out.flush();
            }
            else {
                System.out.println("C: "+line);
                outFile.println("C: "+line);
                System.out.println("S: 500 Unknown command");
                outFile.println("S: 500 Unknown command");
                out.println("500 Unknown command");
                out.flush();
            }
        }
        outFile.close();
        outFileStrm.close();
    }


    public static void main(String[] args) {
        if (args.length < 1) {
            System.out.println("Usage: java SmtpServer address [port]");
            System.exit(1);
        }
        try {
            InetAddress addr = InetAddress.getByName(args[0]);
            int port = 25;
            if (args.length > 1) {
                port = Integer.parseInt(args[1]);
            }
            ServerSocket serverSocket = new ServerSocket(port, 5, addr);
            while (true) {
                System.out.println("Waiting for connection on "+
                    addr.getHostAddress()+":"+port);
                Socket socket = serverSocket.accept();
                try {
                    SmtpServer serverThread = new SmtpServer(socket, addr);
                    serverThread.start();
                }
                catch (Exception e) {
                    System.out.println("Whoops!");
                }
            }
        }
        catch (Exception e) {
            System.out.println(e);
        }
    }

}
