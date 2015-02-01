package com.jdodev.updtest;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.MulticastSocket;

import android.app.Activity;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.util.Log;


public class UDPClient extends Thread
{
	public final static String TAG = "UDPCLIENT";
	
	public final static int UDP_PORT = 1234;
	public final static String MULTICAST_ADDR = "225.0.0.37";
	private final static int MAXUDP_PAYLOADSZ = 65527;
	
	public interface Listener {
		public void OnThreadStart();
		public void OnDataReceived(UDPFrame frame);
		public void OnThreadEnd();
	}
	
	private Activity mCtx = null;
	private Listener mListener = null;
	private WifiManager.WifiLock mLock1 = null;
	private WifiManager.MulticastLock mLock2 = null;
	
	private volatile boolean done = false;
	
	public UDPClient(Activity ctx, Listener listener) {
		mCtx = ctx;
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
		
	public void shutdown() {
	    done = true;
	}
	
	private void end() {
		if (mLock1!=null) mLock1.release();
		mLock1 = null;
		if (mLock2!=null) mLock2.release();
		mLock2 = null;
		if (mListener!=null) mCtx.runOnUiThread(new Runnable() {
			@Override
			public void run() {
			mListener.OnThreadEnd();
		} });
		
		Log.i(TAG,"Rx Thread end");
	}
	
	
	@Override 
	public synchronized void run() { 
		Log.i(TAG,"Rx Thread start");
		MulticastSocket socket = null;
		DatagramPacket packet = null;
		
		// init
		try {
			socket = new MulticastSocket(UDP_PORT);
			socket.joinGroup(InetAddress.getByName(MULTICAST_ADDR));
			socket.setSoTimeout(3000);
			packet = new DatagramPacket( new byte[MAXUDP_PAYLOADSZ], MAXUDP_PAYLOADSZ);
		} catch (IOException e) {
			e.printStackTrace();
			Log.e(TAG,e.toString());
			end(); 
		}
		
		if (mListener!=null) mCtx.runOnUiThread(new Runnable() {
			@Override
			public void run() {
			mListener.OnThreadStart();
		} });
		
		// read
		boolean first = true;
		long old_framenumber = 0;
		UDPFrame.ClearStats();
		while (!done) {
			try {
		           
				socket.receive(packet);
				final UDPFrame frame = new UDPFrame();
				if (frame.parse(packet)) {
					
					if (first) { 
						first = false;
						old_framenumber = frame.framenumber;
					} else {
						if (old_framenumber - frame.framenumber > (65535/2)) {
							Log.d(TAG,"frame number rollover "+old_framenumber+" "+frame.framenumber);
							first = true;
							old_framenumber = 0;
							UDPFrame.ClearStatsRollover();							
						} else {
							frame.computestats(old_framenumber);
							if (frame.isValid()) old_framenumber = frame.framenumber;
						}
					}
					
					if (frame.isValid() && mListener!=null) mCtx.runOnUiThread(new Runnable() {
						@Override
						public void run() {
							mListener.OnDataReceived(frame);
					} });					
				}
				
			} catch (IOException e) {
				Log.e(TAG,e.toString());
			}
		}
		
		
		// end
		socket.close();
		socket = null;
		packet = null;
		Log.i(TAG,"Rx Thread proper end");
		end(); 		
	} 
}