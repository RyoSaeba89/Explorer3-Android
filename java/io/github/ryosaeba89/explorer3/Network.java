package io.github.ryosaeba89.explorer3;

import android.content.Context;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Collections;

/**
 * The network Android is connected to (user's choice: Android's own Wi-Fi
 * settings, never a network chosen or created by the game): the Wi-Fi first,
 * otherwise a wired interface (the OUYA's Ethernet). Mobile data does not
 * count.
 */
final class Network {
    private Network() {}

    /** { name, IPv4 address }, both empty when not connected. */
    static String[] current(Context context) {
        try {
            WifiManager wifi = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            WifiInfo info = wifi == null ? null : wifi.getConnectionInfo();
            int a = info == null ? 0 : info.getIpAddress();
            if (a != 0) {  // little-endian IPv4
                String ip = (a & 0xFF) + "." + (a >> 8 & 0xFF) + "." + (a >> 16 & 0xFF) + "." + (a >>> 24);
                return new String[] {ssid(info), ip};
            }
        } catch (Exception e) {
            // no Wi-Fi service
        }
        String ip = wiredAddress();
        return new String[] {ip.length() == 0 ? "" : "Ethernet", ip};
    }

    private static String ssid(WifiInfo info) {
        String s = info.getSSID();
        if (s != null && s.length() > 1 && s.startsWith("\"") && s.endsWith("\"")) {
            s = s.substring(1, s.length() - 1);  // quoted from Android 4.2 on
        }
        return s == null || s.length() == 0 || s.contains("unknown ssid") ? "Wi-Fi" : s;
    }

    /** IPv4 address of an active interface other than loopback and mobile data, or "". */
    private static String wiredAddress() {
        try {
            for (NetworkInterface ni : Collections.list(NetworkInterface.getNetworkInterfaces())) {
                String n = ni.getName();
                if (ni.isLoopback() || !ni.isUp() || n.startsWith("rmnet") || n.startsWith("ppp")) {
                    continue;
                }
                for (InetAddress a : Collections.list(ni.getInetAddresses())) {
                    if (a instanceof Inet4Address && !a.isLoopbackAddress()) {
                        return a.getHostAddress();
                    }
                }
            }
        } catch (Exception e) {
            // no interface list
        }
        return "";
    }
}
