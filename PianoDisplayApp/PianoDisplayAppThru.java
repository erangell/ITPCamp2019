import javax.sound.midi.*;
import java.io.*;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class PianoDisplayAppThru
{
	static JFrame f = new JFrame("MIDI IN Display");
	static MidiDisplayPanel midiDisplayPanel;
	static MidiDevice myMidiInDevice;
	static MidiDevice myMidiOutDevice;
	static boolean _running = false;
	static Thread _looper;
        static int _note2show = 109;
	static int[] _NoteVelocities;

	public  static void main(String[] args) 
	{
		_NoteVelocities = new int[128];
		for (int i=0; i<128; i++)
		{
			_NoteVelocities[i] = 0;
		}
		_NoteVelocities[_note2show] = 127;

		PianoDisplayAppThru pianoDisplayApp = new PianoDisplayAppThru();
		pianoDisplayApp.go();
	}

	public void go()
	{
		setUpGui();
		setUpMidi();
		System.out.println("MIDI setup complete");		
	}

	public void setUpGui()
	{
		midiDisplayPanel = new MidiDisplayPanel();
		midiDisplayPanel.start();
		f.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
		f.addWindowListener(   
		      new java.awt.event.WindowAdapter()   
      			{  
			        public void windowClosing( java.awt.event.WindowEvent e )   
			        {  
			          System.out.println( "good bye" );  
				  myMidiInDevice.close();  
			          System.exit( 0 );  
			        }  
			}  
		    );  


		f.setContentPane(midiDisplayPanel);
		f.setBounds(10,10,1000,750);
		f.setVisible(true);
	}

	public void setUpMidi()
	{
		MidiDevice.Info[] myMidiDeviceInfo = MidiSystem.getMidiDeviceInfo();
		int numDevs = myMidiDeviceInfo.length;
		String[] allDevices = new String[numDevs];
		boolean[] allDevInput = new boolean[numDevs];
		boolean[] allDevOutput = new boolean[numDevs];
		int numIn = 0;   int numOut = 0;
		for (int i = 0; i < numDevs; i++)
		{	try
			{	MidiDevice device = MidiSystem.getMidiDevice(myMidiDeviceInfo[i]);
				allDevices[i] = myMidiDeviceInfo[i].getName();
				if (device.getMaxTransmitters() != 0)
				{   allDevInput[i] = true;
			    		numIn++;
				}
				if (device.getMaxReceivers() != 0)
				{   allDevOutput[i] =  true;	
			    		numOut++;
				}
			}
			catch (MidiUnavailableException e)
			{	e.printStackTrace();
				numDevs = 0;
			}
		}
		String[] midiInDevices = new String[numIn];
		String[] midiOutDevices = new String[numOut];
		int[] midiInDeviceNums = new int[numIn];
		int[] midiOutDeviceNums = new int[numOut];
		int ixIn = 0;  int ixOut = 0;  	
		int firstNonJavaMidiInDev = 0;
		int firstNonJavaMidiOutDev = 0;
		boolean foundFirst = false;	
		boolean foundFirstOut = false;
		for (int j = 0; j < numDevs; j++)
		{
			if (allDevInput[j])
			{       midiInDevices[ixIn] = allDevices[j];
				if ((allDevices[j].startsWith("USB Uno")) && (!foundFirst))
				{
					firstNonJavaMidiInDev = ixIn;
					foundFirst = true;
				}
		     		midiInDeviceNums[ixIn] = j;
				System.out.println("Midi IN Device["+j+"]="+allDevices[j]);
		     		ixIn++;
			}
			if (allDevOutput[j])
			{    	midiOutDevices[ixOut] = allDevices[j];

				if ((allDevices[j].startsWith("USB Uno")) && (!foundFirstOut))
				{
					firstNonJavaMidiOutDev = ixOut;
					foundFirstOut = true;
				}
		     		midiOutDeviceNums[ixOut] = j;
				System.out.println("Midi OUT Device["+j+"]="+allDevices[j]);	
				ixOut++;
			}				
		} // for

		int currentMidiInDeviceNum = midiInDeviceNums[firstNonJavaMidiInDev];
		int currentMidiOutDeviceNum = midiOutDeviceNums[firstNonJavaMidiOutDev];

		try 
		{   
		    myMidiInDevice = MidiSystem.getMidiDevice( myMidiDeviceInfo[currentMidiInDeviceNum] );
		    myMidiOutDevice = MidiSystem.getMidiDevice( myMidiDeviceInfo[currentMidiOutDeviceNum] );
		    myMidiInDevice.open();
	    	    System.out.println("Opened myMidiInDevice: "+currentMidiInDeviceNum);	  
		    Transmitter myMidiInTransmitter = myMidiInDevice.getTransmitter();
	    	    System.out.println("Setting receiver to midiDisplayPanel");	  
	    	    myMidiInTransmitter.setReceiver(midiDisplayPanel);
		    myMidiOutDevice.open();
	    	    System.out.println("Opened myMidiOutDevice: "+currentMidiOutDeviceNum);	  
		    Transmitter myMidiThruTransmitter = myMidiInDevice.getTransmitter();
	    	    System.out.println("Got 2nd Transmitter.");	  
		    Receiver myMidiThru = myMidiOutDevice.getReceiver();
	    	    System.out.println("Setting THRU receiver to myMidiOutDevice");	  
	    	    myMidiThruTransmitter.setReceiver(myMidiThru);
		}
		catch (MidiUnavailableException e)	    
		{   
	    	    System.out.println("MidiUnavailableException encountered");	    
		}	 

	}



	class MidiDisplayPanel extends JPanel implements Runnable, Receiver
	{

		public void paintComponent (Graphics g)
		{
				
			int width=80;
			int blackheight=525;
			int whiteheight=175;
			int leftOffset = 5;
			int topOffset = 5;
			int octaveHeight = 60;
			int baseledradius = 50;
			int ledradius=20;
			int rowOffset = 0; //10; for space between white and black notes

			g.setColor(new Color(255,255,255));
			g.fillRect(leftOffset,topOffset,12*width,blackheight+whiteheight);

			g.setColor(new Color(0,0,0));
			g.drawRect(leftOffset,topOffset,12*width,blackheight+whiteheight);

			for (int i=0; i<=11; i++)
			{
				if ((i==1) || (i==3) || (i==6) || (i==8) || (i==10))
				{
					g.fillRect(leftOffset + i*width,topOffset,width,blackheight);
				}
			}

			g.drawLine(leftOffset+(int)(width*1.67),topOffset+blackheight
			,leftOffset+(int)(width*1.67),topOffset+blackheight+whiteheight);			

			g.drawLine(leftOffset+(int)(width*3.33),topOffset+blackheight
			,leftOffset+(int)(width*3.33),topOffset+blackheight+whiteheight);

			g.drawLine(leftOffset+(int)(width*5),topOffset
			,leftOffset+(int)(width*5),topOffset+blackheight+whiteheight);

			g.drawLine(leftOffset+(int)(width*6.67),topOffset+blackheight
			,leftOffset+(int)(width*6.67),topOffset+blackheight+whiteheight);

			g.drawLine(leftOffset+(int)(width*8.5),topOffset+blackheight
			,leftOffset+(int)(width*8.5),topOffset+blackheight+whiteheight);

			g.drawLine(leftOffset+(int)(width*10.33),topOffset+blackheight
			,leftOffset+(int)(width*10.33),topOffset+blackheight+whiteheight);

			int testNote = 108;

			
			if (_NoteVelocities[testNote ] > 0)
			{
				g.setColor(new Color(255,255,0));
				ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
			}
			else
			{
				g.setColor(new Color(255,255,255));
			}


			g.fillOval( leftOffset + (int)(width/2)-(ledradius/2)
			, topOffset+rowOffset+(baseledradius-ledradius)/2, ledradius,ledradius);
			
			testNote-- ;


			for (int octave=1; octave<=7; octave++)
			{

				g.setColor(new Color(0,0,0));
				g.drawLine(leftOffset,topOffset+rowOffset+ octave * octaveHeight
					,leftOffset+(int)(width*12),topOffset+rowOffset+ octave * octaveHeight);


				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(255,0,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(255,255,255));
				}
				g.fillOval( leftOffset + (int)(width*11.5)-(ledradius/2)
				, topOffset+rowOffset+ octave * octaveHeight+(baseledradius-ledradius)/2, ledradius,ledradius);

				testNote-- ;

				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(0,255,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(0,0,0));
				}
				g.fillOval (leftOffset + (int)(width*10.5)-(ledradius/2)
				, topOffset + octave * octaveHeight+(baseledradius-ledradius)/2,ledradius,ledradius);
				
				testNote-- ;

				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(255,0,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(255,255,255));
				}
				g.fillOval( leftOffset + (int)(width*9.5)-(ledradius/2)
				, topOffset+rowOffset+ octave * octaveHeight+(baseledradius-ledradius)/2, ledradius,ledradius);
				
				testNote-- ;
				
				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(0,255,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(0,0,0));
				}
				g.fillOval (leftOffset + (int)(width*8.5)-(ledradius/2)
				, topOffset + octave * octaveHeight+(baseledradius-ledradius)/2,ledradius,ledradius);
				
				testNote-- ;

				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(255,0,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(255,255,255));
				}					
				g.fillOval( leftOffset + (int)(width*7.5)-(ledradius/2)
				, topOffset+rowOffset+ octave * octaveHeight+(baseledradius-ledradius)/2, ledradius,ledradius);
				
				testNote-- ;

				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(0,255,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(0,0,0));
				}
				g.fillOval (leftOffset + (int)(width*6.5)-(ledradius/2)
				, topOffset + octave * octaveHeight+(baseledradius-ledradius)/2,ledradius,ledradius);
				testNote-- ;

				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(255,0,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(255,255,255));
				}
				g.fillOval( leftOffset + (int)(width*5.5)-(ledradius/2)
				, topOffset+rowOffset+ octave * octaveHeight+(baseledradius-ledradius)/2, ledradius,ledradius);

				testNote-- ;

				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(255,0,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(255,255,255));
				}
				g.fillOval( leftOffset + (int)(width*4.5)-(ledradius/2)
				, topOffset+rowOffset+ octave * octaveHeight+(baseledradius-ledradius)/2, ledradius,ledradius);

				testNote-- ;

				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(0,255,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(0,0,0));
				}
				g.fillOval (leftOffset + (int)(width*3.5)-(ledradius/2)
				, topOffset + octave * octaveHeight+(baseledradius-ledradius)/2,ledradius,ledradius);
				
				testNote-- ;
				
				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(255,0,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(255,255,255));
				}
				g.fillOval( leftOffset + (int)(width*2.5)-(ledradius/2)
				, topOffset+rowOffset+ octave * octaveHeight+(baseledradius-ledradius)/2, ledradius,ledradius);

				testNote-- ;
				
				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(0,255,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(0,0,0));
				}
				g.fillOval (leftOffset + (int)(width*1.5)-(ledradius/2)
				, topOffset + octave * octaveHeight+(baseledradius-ledradius)/2,ledradius,ledradius);
				
				testNote-- ;
				
				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(255,0,0));
					ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
				}
				else
				{
					g.setColor(new Color(255,255,255));
				}
				g.fillOval( leftOffset + (int)(width*0.5)-(ledradius/2)
				, topOffset+rowOffset+ octave * octaveHeight+(baseledradius-ledradius)/2, ledradius,ledradius);
				
				testNote-- ;

			}	
			int octave = 8;	
			g.setColor(new Color(0,0,0));
			g.drawLine(leftOffset,topOffset+rowOffset+ octave * octaveHeight
					,leftOffset+(int)(width*12),topOffset+rowOffset+ octave * octaveHeight);

			if (_NoteVelocities[testNote ] > 0)
			{
				g.setColor(new Color(255,0,0));
				ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
			}
			else
			{
				g.setColor(new Color(255,255,255));
			}
			g.fillOval( leftOffset + (int)(width*11.5)-(ledradius/2)
			, topOffset+rowOffset+ octave * octaveHeight+(baseledradius-ledradius)/2, ledradius,ledradius);

			testNote-- ;

			if (_NoteVelocities[testNote ] > 0)
			{
				g.setColor(new Color(0,255,0));
				ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
			}
			else
			{
				g.setColor(new Color(0,0,0));
			}
			g.fillOval (leftOffset + (int)(width*10.5)-(ledradius/2)
			, topOffset + octave * octaveHeight+(baseledradius-ledradius)/2,ledradius,ledradius);
				
			testNote-- ;

			if (_NoteVelocities[testNote ] > 0)
			{
				g.setColor(new Color(255,0,0));
				ledradius=(int)(baseledradius * Math.sqrt(_NoteVelocities[testNote])/Math.sqrt(128));
			}
			else
			{
				g.setColor(new Color(255,255,255));
			}
			g.fillOval( leftOffset + (int)(width*9.5)-(ledradius/2)
			, topOffset+rowOffset+ octave * octaveHeight+(baseledradius-ledradius)/2, ledradius,ledradius);


		}

//------------------------- IMPLEMENTATION OF RUNNABLE INTERFACE ---------------

		public void start() {
		        if(_looper == null) {
        		    _running = true;
	        	    _looper = new Thread(this);
	        	    _looper.start();
		        }
		}

	        public void stop() {
		        _running = false;
		}

	        public void run() {
		        try {
		        	while(_running) {

					/*	
					_NoteVelocities[_note2show] = 0;
					_note2show --;					
					if (_note2show  < 21)
					{
						_note2show = 109;
					}
					_NoteVelocities[_note2show] = 127;
					}
					*/

					repaint();
					_looper.sleep(50);		
				}
	        	} catch(InterruptedException e) {
		            _running = false;
		        }
	        }
//------------------------- IMPLEMENTATION OF RUNNABLE INTERFACE ---------------


//------------------------- IMPLEMENTATION OF RECEIVER INTERFACE ---------------
		public void send(MidiMessage message, long lTimeStamp)
		{
			if ((message instanceof ShortMessage))
			{
				ShortMessage myShortMessage = (ShortMessage) message;
				if (myShortMessage.getCommand() == 0x90)
				{			    	
			    		int msgChnl = myShortMessage.getChannel();
			    		int msgNote = myShortMessage.getData1();
			    		int msgVel = myShortMessage.getData2();
					_NoteVelocities[msgNote] = msgVel;
				}
				else if (myShortMessage.getCommand() == 0x80)
				{
			    		int msgChnl = myShortMessage.getChannel();
			    		int msgNote = myShortMessage.getData1();
			    		int msgVel  = 0;
					_NoteVelocities[msgNote] = msgVel;
				}
			}
		}

		public void close()
		{

		}
//------------------------- IMPLEMENTATION OF RECEIVER INTERFACE ---------------


	}
}