var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

var WEATHER_INTERVAL = 30 * 60 * 1000;
var LOCATION_FALLBACK = "LOCALISATION";

function xhrGet(url, cb) {
  var req = new XMLHttpRequest();
  req.onload = function() {
    if (this.status >= 200 && this.status < 300) cb(null, this.responseText);
    else cb(new Error("HTTP " + this.status));
  };
  req.onerror = function() { cb(new Error("Erreur réseau")); };
  req.open("GET", url);
  req.send();
}

function sendWeather(city, temp) {
  Pebble.sendAppMessage({
    "TEMPERATURE": Math.round(temp),
    "LOCATION": city || LOCATION_FALLBACK
  }, function() {
    console.log("Météo envoyée : " + (city || LOCATION_FALLBACK) + " / " + Math.round(temp) + " C");
  }, function(err) {
    console.log("Erreur envoi météo : " + JSON.stringify(err));
  });
}

function updateWeather() {
  navigator.geolocation.getCurrentPosition(function(pos) {
    var lat = pos.coords.latitude;
    var lon = pos.coords.longitude;

    var geoUrl =
      "https://api.bigdatacloud.net/data/reverse-geocode-client" +
      "?latitude=" + encodeURIComponent(lat) +
      "&longitude=" + encodeURIComponent(lon) +
      "&localityLanguage=fr";

    xhrGet(geoUrl, function(err, txt) {
      var city = "LOCALISATION";
      if (!err) {
        try {
          var g = JSON.parse(txt);
          city = (g.city || g.locality || g.localityInfo && g.localityInfo.administrative &&
                  g.localityInfo.administrative[0] && g.localityInfo.administrative[0].name ||
                  g.principalSubdivision || city).toUpperCase();
          if (city.length > 18) city = city.substring(0, 18);
        } catch(e) {}
      }

      var weatherUrl =
        "https://api.open-meteo.com/v1/forecast" +
        "?latitude=" + encodeURIComponent(lat) +
        "&longitude=" + encodeURIComponent(lon) +
        "&current=temperature_2m&timezone=auto";

      xhrGet(weatherUrl, function(werr, wtxt) {
        if (werr) {
          Pebble.sendAppMessage({"LOCATION": city || LOCATION_FALLBACK});
          return;
        }
        try {
          var w = JSON.parse(wtxt);
          if (w.current && typeof w.current.temperature_2m !== "undefined") {
            sendWeather(city, w.current.temperature_2m);
          }
        } catch(e) {}
      });
    });
  }, function(err) {
    Pebble.sendAppMessage({"LOCATION": LOCATION_FALLBACK});
  }, {
    enableHighAccuracy:false,
    maximumAge:600000,
    timeout:15000
  });
}

Pebble.addEventListener("ready", function() {
  console.log("Quartz LCD 1.5 JS ready");
  updateWeather();
  setInterval(updateWeather, WEATHER_INTERVAL);
});

Pebble.addEventListener("appmessage", function(e) {
  if (e.payload && e.payload.REQUEST_WEATHER) updateWeather();
});