package com.rejment.restartme;

import java.io.IOException;

public class TestEnv extends ProcessEnvironment {
    public int messageReceived(String msg) {
        System.out.println("Received message: " + msg);
        return 555;
    }

    public static void main(String[] args) throws InterruptedException, IOException {

        ProcessEnvironment pe = new TestEnv();
        System.out.println("ENV CREATED");
        int me = pe.register("test");
        System.out.println("REGISTERED ME AS: " + me);
        int p[] = pe.getProcesses("test");
        System.out.println("got Processes:" + p.length);
        for (int i=0; i<p.length; i++) {
            if (p[i] != me) {
                System.out.println("notifying: " + p[i]);
                int x = pe.sendMessage(p[i], "hello world");
                System.out.println("x=" + x);
            }
        }

        Thread.sleep(1000*40);
    }
}
