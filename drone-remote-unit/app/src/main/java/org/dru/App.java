package org.dru;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.scene.Scene;
import javafx.scene.layout.Background;
import javafx.scene.layout.BackgroundImage;
import javafx.scene.layout.BackgroundPosition;
import javafx.scene.layout.BackgroundRepeat;
import javafx.scene.layout.BackgroundSize;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextArea;
import javafx.scene.image.Image;
import javafx.geometry.Insets;

public class App extends Application {

    // Label dinamiche
    private final Label lblStatoDrone = new Label("Stato Drone: -");
    private final Label lblStatoHangar = new Label("Stato Hangar: -");
    private final Label lblDistanza = new Label("Distanza: -");
    private final Label outArduino = new Label("");

    // Console / log (ora campo per poter scrivere da onSerialDataReceived)
    private final TextArea console = new TextArea();

    /**
     * Callback chiamato quando arrivano dati dalla seriale.
     * Questo metodo è chiamato da ArduinoController (thread di lettura).
     * Tutto l'aggiornamento UI è dentro Platform.runLater e protetto da try/catch.
     */
    private void onSerialDataReceived(String data) {
        // Protezione: se callback riceve null, logghiamo e ritorniamo
        if (data == null) {
            appendLog("Ricevuto null dalla seriale");
            return;
        }

        final String payload = data; // non modificare la stringa sul thread della seriale

        Platform.runLater(() -> {
            try {
                // sanitize
                String d = payload.replace("\r", "").replace("\n", "").trim();
                if (d.isEmpty()) {
                    // ignora messaggi vuoti
                    return;
                }

                // Mostra raw
                outArduino.setText("RX: " + d);
                appendLog("RX: " + d);

                // Parser semplice
                if (d.startsWith("DRONE:")) {
                    String stato = d.substring(6).trim();
                    lblStatoDrone.setText("Stato Drone: " + stato);
                } else if (d.startsWith("HANGAR:")) {
                    String stato = d.substring(7).trim();
                    lblStatoHangar.setText("Stato Hangar: " + stato);
                } else if (d.startsWith("DISTANZA:")) {
                    String dist = d.substring(9).trim();
                    lblDistanza.setText("Distanza: " + dist + " cm");
                } else {
                    // Messaggio non riconosciuto
                    appendLog("Messaggio non riconosciuto: " + d);
                }

            } catch (Exception ex) {
                // Logghiamo lo stacktrace nella console UI
                appendLog("Errore in onSerialDataReceived: " + ex.toString());
                for (StackTraceElement ste : ex.getStackTrace()) {
                    appendLog("  at " + ste.toString());
                }
            }
        });
    }

    @Override
    public void start(Stage stage) {

        String portName = "COM4";

        // ArduinoController riceve la callback onSerialDataReceived
        ArduinoController controller = new ArduinoController(
                this::onSerialDataReceived,
                portName
        );

        console.setEditable(false);
        console.setWrapText(true);

        Button connectBtn = new Button("Connetti a " + portName);
        Button disconnectBtn = new Button("Disconnetti");
        Button takeOffBtn = new Button("Decollo");
        Button landBtn = new Button("Atterraggio");
        Button ResetBtn = new Button("Reset");


        // Pulsanti
        connectBtn.setOnAction(e -> {
            appendLog("Tentativo di connessione a " + portName);
            controller.connect();
        });
        disconnectBtn.setOnAction(e -> {
            appendLog("Richiesta disconnessione");
            controller.disconnect();
        });
        takeOffBtn.setOnAction(e -> {
            appendLog("Invio TAKE_OFF");
            controller.takeOff();
        });
        landBtn.setOnAction(e -> {
            appendLog("Invio LAND");
            controller.land();
        });
        ResetBtn.setOnAction(e -> {
            appendLog("Invio Reset");
            controller.Reset();
        });

        VBox root = new VBox(10,
                connectBtn,
                disconnectBtn,
                takeOffBtn,
                landBtn,
                ResetBtn,
                lblStatoDrone,
                lblStatoHangar,
                lblDistanza,
                outArduino,
                console
        );
        root.setPadding(new Insets(12));

        Image img = new Image(getClass().getResource("/img/hangar.jpg").toExternalForm()); // o path relativo
        BackgroundImage bgImg = new BackgroundImage(
        img,
        BackgroundRepeat.NO_REPEAT,
        BackgroundRepeat.NO_REPEAT,
        BackgroundPosition.CENTER,
        new BackgroundSize(
                BackgroundSize.AUTO, BackgroundSize.AUTO,
                false, false, true, true
        )
        );

        root.setBackground(new Background(bgImg));
        Scene scene = new Scene(root,900,600);
        stage.setTitle("JavaFX ↔ Arduino (DRU System)");
        scene.getStylesheets().add(getClass().getResource("/style.css").toExternalForm());
        stage.setScene(scene);
        stage.show();
    }

    // Metodo helper per aggiungere righe alla console (UI thread safe)
    private void appendLog(String s) {
        // Se siamo già sulla UI thread
        if (Platform.isFxApplicationThread()) {
            console.appendText(s + "\n");
        } else {
            Platform.runLater(() -> console.appendText(s + "\n"));
        }
    }

    public static void main(String[] args) {
        launch(args);
    }
}
