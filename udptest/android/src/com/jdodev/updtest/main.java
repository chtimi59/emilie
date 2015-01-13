package com.jdodev.updtest;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.MulticastSocket;
import java.net.SocketException;

import android.app.Activity;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;

public class main extends Activity {
	
	final static int UDP_PORT = 1234;
	final static String MULTICAST_ADDR = "225.0.0.37";
	
	
	WifiManager.WifiLock lock1;
	WifiManager.MulticastLock lock2;
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
		
        
        WifiManager wifi = (WifiManager)getSystemService(Context.WIFI_SERVICE);
        if (wifi != null){
        	lock1 = wifi.createWifiLock("mylock");
            lock1.acquire();
            lock2=wifi.createMulticastLock("mylock2");
            lock2.acquire();
            new Thread(new Client()).start(); 	
        }
    }
	
	
	public class Client implements Runnable
	{ 
		@Override 
		public void run() { 
			try { 
				 

				MulticastSocket socket = new MulticastSocket(UDP_PORT);;
				socket.joinGroup(InetAddress.getByName(MULTICAST_ADDR));
				socket.setSoTimeout(5000);
				
				byte[] buf = new byte[1024];
				DatagramPacket packet = new DatagramPacket(buf, buf.length);
				  
				socket.receive(packet); 
				socket.close();
				
				String text = new String(buf, 0, packet.getLength());
				Log.d("ERR",text);
				SystemClock.sleep(500);
				new Thread(new Client()).start(); 	
				
			}  catch (SocketException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				Log.e("ERR",e.toString());
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				Log.e("ERR",e.toString());
			} 
		} 
	}
	
	
	
	
}
