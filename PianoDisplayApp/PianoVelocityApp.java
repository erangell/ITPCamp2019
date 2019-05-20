import javax.sound.midi.*;
import java.io.*;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class PianoVelocityApp
{
	static JFrame f = new JFrame("MIDI IN Display");
	static MidiDisplayPanel midiDisplayPanel;
	static MidiDevice myMidiInDevice;
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

		PianoVelocityApp PianoVelocityApp = new PianoVelocityApp();
		PianoVelocityApp.go();
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
		boolean foundFirst = false;
		for (int j = 0; j < numDevs; j++)
		{
			if (allDevInput[j])
			{    	midiInDevices[ixIn] = allDevices[j];
				if ((!allDevices[j].startsWith("Java")) && (!foundFirst))
				{
					firstNonJavaMidiInDev = ixIn;
					foundFirst = true;
				}
				midiInDeviceNums[ixIn] = j;
				System.out.println("Midi IN Device["+j+"]="+allDevices[j]);
		     		ixIn++;
			}
			if (allDevOutput[j])
			{    midiOutDevices[ixOut] = allDevices[j];
		     		midiOutDeviceNums[ixOut] = j;
		     		ixOut++;
			}				
		} // for

		int currentMidiInDeviceNum = midiInDeviceNums[firstNonJavaMidiInDev];


		try 
		{   
		    myMidiInDevice = MidiSystem.getMidiDevice( myMidiDeviceInfo[currentMidiInDeviceNum] );
		    myMidiInDevice.open();
		    Transmitter myMidiInTransmitter = myMidiInDevice.getTransmitter();
	    	    myMidiInTransmitter.setReceiver(midiDisplayPanel);
	    	    System.out.println("Opened myMidiInDevice: "+currentMidiInDeviceNum);	  
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
				

			int leftOffset = 5;
			int topOffset = 5;
			int frameHeight= 700;
			int frameWidth= 1100;
			int keyWidth = frameWidth / 88;
			int velUnit = frameHeight / 128;


			g.setColor(new Color(255,255,255));
			g.fillRect(leftOffset,topOffset,frameWidth,frameHeight);

			g.setColor(new Color(0,0,0));
			g.drawRect(leftOffset,topOffset,frameWidth,frameHeight);

			for (int testNote=21; testNote <= 108; testNote++)
			{
				if (_NoteVelocities[testNote ] > 0)
				{
					g.setColor(new Color(0,0,255));
					g.fillRect(leftOffset + keyWidth * (testNote-21)
						, topOffset + (127-_NoteVelocities[testNote])*velUnit
						,keyWidth,_NoteVelocities[testNote]*velUnit);
				}
			}

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