/**
 * @file gps_tracker.cpp
 * @author Rennan Cockles (https://github.com/rennancockles)
 * @brief GPS tracker
 * @version 0.1
 * @date 2024-11-20
 */

#include "gps_tracker.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "current_year.h"

#define MAX_WAIT 5000

GPSTracker::GPSTracker() { setup(); }

GPSTracker::~GPSTracker() {
    add_final_file_data();
    if (gpsConnected) end();
    ioExpander.turnPinOnOff(IO_EXP_GPS, LOW);
}

void GPSTracker::setup() {
    ioExpander.turnPinOnOff(IO_EXP_GPS, HIGH);
    #if (defined(HAS_TFT) || defined(HAS_SCREEN))
    display_banner();
    padprintln("Initializing...");
    #endif

    if (!begin_gps()) return;

    return loop();
}

bool GPSTracker::begin_gps() {
    GPSserial.begin(bruceConfig.gpsBaudrate, SERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX);

    #if (defined(HAS_TFT) || defined(HAS_SCREEN))
    int count = 0;
    padprintln("Waiting for GPS data");
    while (GPSserial.available() <= 0) {
        if (check(EscPress)) {
            end();
            return false;
        }
        displayTextLine("Waiting GPS: " + String(count) + "s");
        count++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    #endif

    gpsConnected = true;
    return true;
}

void GPSTracker::end() {
    GPSserial.end();

    returnToMenu = true;
    gpsConnected = false;
}

void GPSTracker::loop() {
    int count = 0;
    returnToMenu = false;
    while (1) {
        display_banner();

        if (check(EscPress) || returnToMenu) return end();

        if (GPSserial.available() > 0) {
            count = 0;
            while (GPSserial.available() > 0) gps.encode(GPSserial.read());

            if (gps.location.isUpdated()) {
                #if (defined(HAS_TFT) || defined(HAS_SCREEN))
                padprintln("GPS location updated");
                #endif
                set_position();
                add_coord();
            } else {
                #if (defined(HAS_TFT) || defined(HAS_SCREEN))
                padprintln("GPS location not updated");
                #endif
                dump_gps_data();

                if (filename == "" && gps.date.year() >= CURRENT_YEAR && gps.date.year() < CURRENT_YEAR + 5)
                    create_filename();
            }
        } else {
            if (count > 5) {
                #if (defined(HAS_TFT) || defined(HAS_SCREEN))
                displayError("GPS not Found!");
                #endif
                return end();
            }
            #if (defined(HAS_TFT) || defined(HAS_SCREEN))
            padprintln("No GPS data available");
            #endif
            count++;
        }

        int tmp = millis();
        while (millis() - tmp < MAX_WAIT && !gps.location.isUpdated()) {
            if (check(EscPress) || returnToMenu) return end();
        }
    }
}

void GPSTracker::set_position() {
    double lat = gps.location.lat();
    double lng = gps.location.lng();

    if (initial_position_set) distance += gps.distanceBetween(cur_lat, cur_lng, lat, lng);
    else initial_position_set = true;

    cur_lat = lat;
    cur_lng = lng;
}

void GPSTracker::display_banner() {
    #if (defined(HAS_TFT) || defined(HAS_SCREEN))
    drawMainBorderWithTitle("GPS Tracker");
    padprintln("");

    if (gpsCoordCount > 0) {
        padprintln("File: " + filename.substring(0, filename.length() - 4), 2);
        padprintln("GPS Coordinates: " + String(gpsCoordCount), 2);
        padprintf(2, "Distance: %.2fkm\n", distance / 1000);
    }

    padprintln("");
    #endif
}

void GPSTracker::dump_gps_data() {
    #if (defined(HAS_TFT) || defined(HAS_SCREEN))
    if (!date_time_updated && (!gps.date.isUpdated() || !gps.time.isUpdated())) {
        padprintln("Waiting for valid GPS data");
        return;
    }
    date_time_updated = true;
    padprintf(2, "Date: %02d-%02d-%02d\n", gps.date.year(), gps.date.month(), gps.date.day());
    padprintf(2, "Time: %02d:%02d:%02d\n", gps.time.hour(), gps.time.minute(), gps.time.second());
    padprintf(2, "Sat:  %d\n", gps.satellites.value());
    padprintf(2, "HDOP: %.2f\n", gps.hdop.hdop());
    #endif
}

void GPSTracker::create_filename() {
    char timestamp[20];
    sprintf(
        timestamp,
        "%02d%02d%02d_%02d%02d%02d",
        gps.date.year() % 100,
        gps.date.month() % 100,
        gps.date.day() % 100,
        gps.time.hour() % 100,
        gps.time.minute() % 100,
        gps.time.second() % 100
    );
    filename = String(timestamp) + "_gps_tracker.gpx";
}

void GPSTracker::add_initial_file_data(File file) {
    file.println("<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>");
    file.println("<?xml-stylesheet type=\"text/xsl\" href=\"details.xsl\"?>");
    file.println("<gpx");
    file.println("  version=\"1.1\"");
    file.println("  creator=\"Bruce Firmware\"");
    file.println("  xmlns=\"http://www.topografix.com/GPX/1/1\"");
    file.println("  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
    file.println(
        "  xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\""
    );
    file.println(">");
    file.println("  <metadata>");
    file.println("    <name>Bruce GPS Tracker</name>");
    file.println("    <desc>GPS Tracker using Bruce Firmware</desc>");
    file.println("    <link href=\"https://bruce.computer\">");
    file.println("      <text>Bruce Website</text>");
    file.println("    </link>");
    file.println("  </metadata>");
    file.println("  <trk>");
    file.println("    <name>Bruce Route</name>");
    file.println("    <desc>GPS route captured by Bruce firmware</desc>");
    file.println("    <trkseg>");
}

void GPSTracker::add_final_file_data() {
    FS *fs;
    if (!getFsStorage(fs)) return;
    if (filename == "" || !(*fs).exists("/BruceGPS/" + filename)) return;

    File file = (*fs).open("/BruceGPS/" + filename, FILE_APPEND);

    if (!file) return;
    file.println("    </trkseg>");
    file.println("  </trk>");
    file.println("</gpx>");

    file.close();
}

void GPSTracker::add_coord() {
    FS *fs;
    if (!getFsStorage(fs)) {
        #if (defined(HAS_TFT) || defined(HAS_SCREEN))
        padprintln("Storage setup error");
        #endif
        returnToMenu = true;
        return;
    }

    if (filename == "") create_filename();

    if (!(*fs).exists("/BruceGPS")) (*fs).mkdir("/BruceGPS");

    bool is_new_file = false;
    if (!(*fs).exists("/BruceGPS/" + filename)) is_new_file = true;
    File file = (*fs).open("/BruceGPS/" + filename, is_new_file ? FILE_WRITE : FILE_APPEND);

    if (!file) {
        #if (defined(HAS_TFT) || defined(HAS_SCREEN))
        padprintln("Failed to open file for writing");
        #endif
        returnToMenu = true;
        return;
    }

    if (is_new_file) add_initial_file_data(file);

    file.printf("      <trkpt lat=\"%f\" lon=\"%f\">\n", gps.location.lat(), gps.location.lng());
    file.println("        <sym>Waypoint</sym>");
    file.printf("        <ele>%f</ele>\n", gps.altitude.meters());
    file.printf("        <hdop>%f</hdop>\n", gps.hdop.hdop());
    file.printf("        <sat>%d</sat>\n", gps.satellites.value());
    file.println("      </trkpt>");

    gpsCoordCount++;

    file.close();

    #if (defined(HAS_TFT) || defined(HAS_SCREEN))
    padprintf(2, "Coord: %.6f, %.6f\n", gps.location.lat(), gps.location.lng());
    #endif
}
