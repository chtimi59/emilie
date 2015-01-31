package com.jdodev.updtest;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.MulticastSocket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.PowerManager;
import android.util.Log;


public class UDPClient extends Thread
{ 
	private final static int UDP_PORT = 1234;
	private final static String MULTICAST_ADDR = "225.0.0.37";
	private final static int MAXUDP_PAYLOADSZ = 65527;
	
	public interface Listener {
		public void OnThreadStart();
		public void OnDataReceived(UDPFrame frame);
		public void OnThreadEnd();
	}
	
	private Listener mListener = null;
	private WifiManager.WifiLock mLock1 = null;
	private WifiManager.MulticastLock mLock2 = null;
	
	private volatile boolean done = false;
	
	public UDPClient(Context ctx, Listener listener) {
		mListener = listener;
		
		WifiManager wifi = (WifiManager)ctx.getSystemService(Context.WIFI_SERVICE);
        if (wifi == null) { end(); return; }
        
        mLock1 = wifi.createWifiLock("mylock");
        if (mLock1==null) { end(); return; }
        mLock1.acquire();
        
        mLock2=wifi.createMulticastLock("mylock2");
        if (mLock2==null) { end(); return; }
        mLock2.acquire();
        
        done = false;
	}
		
	synchronized public void shutdown() {
	    done = true;
	}
	
	private void end() {
		if (mLock1!=null) mLock1.release();
		mLock1 = null;
		if (mLock2!=null) mLock2.release();
		mLock2 = null;
		if (mListener!=null) {
			synchronized(mListener) {
				mListener.OnThreadEnd();
			}
		}
	}
	
	
	@Override 
	public synchronized void run() { 

		MulticastSocket socket = null;
		DatagramPacket packet = null;
		
		// init
		try {
			socket = new MulticastSocket(UDP_PORT);
			socket.joinGroup(InetAddress.getByName(MULTICAST_ADDR));
			socket.setSoTimeout(10000);
			packet = new DatagramPacket( new byte[MAXUDP_PAYLOADSZ], MAXUDP_PAYLOADSZ);
		} catch (IOException e) {
			e.printStackTrace();
			Log.e("ERR",e.toString());
			end(); 
		}
		

		// read
		boolean first = true;
		long old_framenumber = 0;
				
		while (!done) {
			try {
		           
				socket.receive(packet);
				UDPFrame frame = new UDPFrame();
				if (frame.parse(packet)) {
					
					if (first) { 
						first = false;
					} else {
						frame.computestats(old_framenumber);
					}
					old_framenumber = frame.framenumber;
					
					if (mListener!=null) {
						synchronized(mListener) {
							mListener.OnDataReceived(frame);
						}
					}
				}
				
			} catch (IOException e) {
				Log.e("ERR",e.toString());
			}
		}
		
		
		// end
		socket.close();
		socket = null;
		packet = null;
		end(); 		
	} 
}