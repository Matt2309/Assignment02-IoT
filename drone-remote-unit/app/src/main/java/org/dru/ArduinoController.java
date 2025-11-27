package org.dru;

import com.fazecast.jSerialComm.SerialPort;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

public class ArduinoController extends Thread {
    public static StringBuilder text=new StringBuilder();
    private final Consumer<String> callback;
    private SerialPort serialPort;

    public ArduinoController(Consumer<String> callback) {
        this.callback = callback;
    }

    public List<String> getAvailablePorts() {
        List<String> result = new ArrayList<>();

        for (SerialPort port : SerialPort.getCommPorts()) {
            String systemName = port.getSystemPortName();
            String description = port.getDescriptivePortName();

            result.add(systemName + " (" + description + ")");
        }

        return result;
    }

    public void connect(String portName) {
        String systemPortName = "/dev/"+portName.split(" ")[0];
        System.out.println("Connecting to port " + systemPortName);
        this.serialPort = SerialPort.getCommPort(systemPortName);
        serialPort.setComPortParameters (9600 , Byte.SIZE , SerialPort.ONE_STOP_BIT, SerialPort.NO_PARITY ) ;
        serialPort.setComPortTimeouts (SerialPort.TIMEOUT_WRITE_BLOCKING,0 ,0);
        if (serialPort.openPort()) {
            System.out.println("Porta aperta: " + systemPortName);
        } else {
            System.out.println("Errore apertura porta");
        }

        new Thread(() -> {
            try {
                while (true) {
                    byte[] data = new byte[10];
                    serialPort.readBytes(data,1);
                    if((char)data[0] >= ' ')
                        text.append((char)data[0]);
                    else
                        if((char)data[0]== '\n'){
                            System.out.println("Input from Arduino: " + text);
                            callback.accept(String.valueOf(text));
                            text.setLength(0);
                        }
                    Thread.sleep(20);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            serialPort.closePort();
        }).start();
    }

    public void takeOff() {
        this.write("TAKEOFF");
    }
    public void land() {
        this.write("LAND");
    }

    private void write(String msg) {
        if (serialPort != null && serialPort.isOpen()) {
            msg = msg + "\n";
            serialPort.writeBytes(msg.getBytes(), msg.length());
        }
    }

    public void disconnect() {
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.closePort();
        }
    }

    public void Reset() {
        this.write("RESET");
    }
}
