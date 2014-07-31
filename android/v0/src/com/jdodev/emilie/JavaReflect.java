package com.jdodev.emilie;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.lang.annotation.Annotation;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

import android.net.wifi.WifiManager;
import android.util.Log;

public class JavaReflect {
	
	
	 /** return an Object from an object field
	  * */	
	public static Object GetField(Object obj, String fieldName) {
		
		Field field = null;
        Object out = null;
		try {
			
			field = obj.getClass().getDeclaredField(fieldName);
			field.setAccessible(true);
			out = field.get(obj);
			PrintClass(out.getClass());
			
		} catch (NoSuchFieldException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		} catch (IllegalAccessException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IllegalArgumentException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		return out;
	}
	
	 /** Output a class description into a txt file in /sdcard/ 
	  * */	
	 public static void PrintClass(Class CLS) {
	    	
	    	String name = CLS.toString();
	    	name = name.replace(".", "_");
	    	int sz = name.length();
	    	if (sz>20) name = name.substring(sz-20, sz);
	    	String data = "class: " + CLS.toString();
	    	
	    	data += "\n";
		    data += "*** ANNOTATION ****\n";
		    Annotation[] ann = null;
		    ann = CLS.getAnnotations();
			for(Annotation a : ann) {
				 data += a.toString();
				 data += "\n";
			}
			
			
	        data += "\n";
	        data += "*** FIELDS ****\n";
			Field[] fields = null;
			fields = CLS.getDeclaredFields();
			for(Field f : fields) {
				 data += f.toString();
				 data += "\n";
			}
			
			data += "\n";
		    data += "*** CLASSES ****\n";
			Class[] cla = null;
			cla = CLS.getClasses();
			for(Class c : cla) {
				 data += c.toString();
				 data += "\n";
			}
			
			data += "\n";
			data += "*** METHODS ****\n";
			Method[] met = null;
			met = CLS.getMethods();
			for(Method m : met) {
				 data += m.toString();
				 data += "\n";
			}
			
			File file = new File("/sdcard/" + name + ".txt");
			try{
			    FileOutputStream f1 = new FileOutputStream(file,false); //True = Append to file, false = Overwrite
			    PrintStream p = new PrintStream(f1);
			    p.print(data);
			    p.close();
			    f1.close();
			} catch (FileNotFoundException e) {
				Log.e("ERROR", e.toString());
			} catch (IOException e) {
				Log.e("ERROR", e.toString());
			} 
	    
	    }
}
