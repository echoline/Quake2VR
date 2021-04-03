package org.echoline.quake2vr;

public class Loader {
    static {
        System.loadLibrary("loader");
    }

    public static native boolean nativeLoadLibrary(String name);
}
