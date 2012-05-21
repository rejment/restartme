package com.rejment.restartme;

public abstract class ProcessEnvironment {

    public ProcessEnvironment() {
        this("restartme");
    }

    public ProcessEnvironment(String libname) {
        System.loadLibrary(libname);
    }

	public synchronized native int[] getProcesses(String name);
    public synchronized native int sendMessage(int process, String msg);
	public synchronized native int register(String name);

    public abstract int messageReceived(String msg);
}