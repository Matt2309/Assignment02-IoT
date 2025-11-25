package org.dru;

import com.fazecast.jSerialComm.SerialPort;
import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.image.Image;
import javafx.scene.layout.*;
import javafx.stage.Stage;

public class App extends Application {

    private final Label lblStatoDrone = new Label("Stato Drone: -");
    private final Label lblStatoHangar = new Label("Stato Hangar: -");
    private final Label lblDistanza = new Label("Distanza: -");
    private final Label outArduino = new Label("");

    private final TextArea console = new TextArea();
    private final ArduinoController[] controller = new ArduinoController[1];

    // ----------------- Callback da Arduino ------------------
    private void onSerialDataReceived(String data) {
        if (data == null) {
            appendLog("Seriale nulla");
            return;
        }

        final String payload = data;

        Platform.runLater(() -> {
            try {
                String d = payload.replace("\r", "").replace("\n", "").trim();
                if (d.isEmpty()) return;

                outArduino.setText("RX: " + d);
                appendLog("RX: " + d);

                if (d.startsWith("DRONE:")) {
                    lblStatoDrone.setText("Stato Drone: " + d.substring(6).trim());
                } else if (d.startsWith("HANGAR:")) {
                    lblStatoHangar.setText("Stato Hangar: " + d.substring(7).trim());
                } else if (d.startsWith("DISTANZA:")) {
                    lblDistanza.setText("Distanza: " + d.substring(9).trim() + " cm");
                }
            } catch (Exception ex) {
                appendLog("Errore callback: " + ex);
            }
        });
    }

    // ----------------------- UI -----------------------------
    @Override
    public void start(Stage stage) {

        // *** Sezione Superiore - Porta + Connessione ***
        ComboBox<String> portSelector = new ComboBox<>();
        for (SerialPort p : SerialPort.getCommPorts())
            portSelector.getItems().add(p.getSystemPortName());
        if (!portSelector.getItems().isEmpty())
            portSelector.getSelectionModel().selectFirst();

        Button connectBtn = new Button("Connetti");
        Button disconnectBtn = new Button("Disconnetti");

        HBox topBar = new HBox(10, new Label("Porta:"), portSelector, connectBtn, disconnectBtn);
        topBar.setAlignment(Pos.CENTER_LEFT);
        topBar.setPadding(new Insets(10));
        topBar.setStyle("-fx-background-color: rgba(0,0,0,0.35); -fx-background-radius:10;");


        // **** Pulsanti di comando drone (orizzontali) ***
        Button takeOffBtn = new Button("Decollo");
        Button landBtn = new Button("Atterraggio");
        Button resetBtn = new Button("RESET");

        HBox commandBar = new HBox(20, takeOffBtn, landBtn, resetBtn);
        commandBar.setAlignment(Pos.CENTER);
        commandBar.setPadding(new Insets(15));
        commandBar.setStyle("-fx-background-color: rgba(0,0,0,0.35); -fx-background-radius:10;");


        // **** Sezione Stato ***
        VBox statusBox = new VBox(10,
                lblStatoDrone,
                lblStatoHangar,
                lblDistanza,
                outArduino
        );
        statusBox.setPadding(new Insets(10));
        statusBox.setStyle("-fx-font-size: 16px; -fx-text-fill: white;");
        statusBox.setAlignment(Pos.CENTER_LEFT);


        // **** Console ***
        console.setEditable(false);
        console.setPrefRowCount(10);
        console.setStyle("-fx-control-inner-background: black; -fx-text-fill: #00FF00;");

        VBox centerContent = new VBox(15, statusBox, console);
        centerContent.setPadding(new Insets(10));
        centerContent.setStyle("-fx-background-color: rgba(0,0,0,0.5); -fx-background-radius:10;");


        // **** Contenitore principale ***
        VBox root = new VBox(20, topBar, commandBar, centerContent);
        root.setPadding(new Insets(20));

        // **** Sfondo elegante ***
        Image img = new Image(getClass().getResource("/img/hangar.jpg").toExternalForm());
        BackgroundImage bgImg = new BackgroundImage(
                img,
                BackgroundRepeat.NO_REPEAT, BackgroundRepeat.NO_REPEAT,
                BackgroundPosition.CENTER,
                new BackgroundSize(BackgroundSize.AUTO, BackgroundSize.AUTO, false, false, true, true)
        );
        root.setBackground(new Background(bgImg));


        // ------------------- Eventi Pulsanti -------------------
        connectBtn.setOnAction(e -> {
            String port = portSelector.getValue();
            if (port == null) {
                appendLog("Nessuna porta selezionata");
                return;
            }

            controller[0] = new ArduinoController(this::onSerialDataReceived, port);
            controller[0].connect();
            appendLog("Connesso a " + port);
        });

        disconnectBtn.setOnAction(e -> {
            if (controller[0] != null) controller[0].disconnect();
            appendLog("Disconnesso");
        });

        takeOffBtn.setOnAction(e -> {
            if (controller[0] != null) controller[0].takeOff();
            appendLog("→ TAKE_OFF");
        });

        landBtn.setOnAction(e -> {
            if (controller[0] != null) controller[0].land();
            appendLog("→ LAND");
        });

        resetBtn.setOnAction(e -> {
            if (controller[0] != null) controller[0].Reset();
            appendLog("→ RESET");
        });


        Scene scene = new Scene(root, 900, 600);
        stage.setTitle("DRU System - Interfaccia Hangar Drone");
        stage.setScene(scene);
        stage.show();
    }


    // Aggiunta log thread-safe
    private void appendLog(String s) {
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
