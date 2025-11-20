package org.dru;

import com.fazecast.jSerialComm.SerialPort;

public class ArduinoController {
    private SerialPort serialPort;

    public void connect(String portName) {
        serialPort = SerialPort.getCommPort(portName);
        serialPort.setBaudRate(9600);

        if (serialPort.openPort()) {
            System.out.println("Connesso a " + portName);
        } else {
            System.out.println("Errore: impossibile aprire la porta");
            return;
        }

        new Thread(() -> {
            try {
                while (true) {
                    if (serialPort.bytesAvailable() > 0) {
                        byte[] buffer = new byte[serialPort.bytesAvailable()];
                        serialPort.readBytes(buffer, buffer.length);
                        System.out.println("Arduino dice: " + new String(buffer));
                    }
                    Thread.sleep(20);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
    }

    public void write(String msg) {
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
}
