module.exports = [
  {
    "type": "heading",
    "defaultValue": "Quartz LCD 1.5"
  },
  {
    "type": "text",
    "defaultValue": "Cadran LCD quartz inspiré des montres numériques vintage."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Couleurs"
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Couleur du fond",
        "sunlight": false
      },
      {
        "type": "color",
        "messageKey": "DateColor",
        "defaultValue": "0xFFFFFF",
        "label": "Couleur de la date",
        "sunlight": false
      },
      {
        "type": "color",
        "messageKey": "TimeColor",
        "defaultValue": "0xFFFFFF",
        "label": "Couleur des textes météo/heure",
        "sunlight": false
      },
      {
        "type": "radiogroup",
        "messageKey": "TemperatureUnit",
        "label": "Unité de température",
        "defaultValue": "C",
        "options": [
          {"label": "Degrés Celsius (°C)", "value": "C"},
          {"label": "Degrés Fahrenheit (°F)", "value": "F"}
        ]
      },
      {
        "type": "radiogroup",
        "messageKey": "SegmentColor",
        "label": "Couleur des segments actifs",
        "defaultValue": "gray",
        "options": [
          {"label": "Gris LCD", "value": "gray"},
          {"label": "Vert", "value": "green"},
          {"label": "Bleu", "value": "blue"},
          {"label": "Orange", "value": "orange"},
          {"label": "Jaune", "value": "yellow"},
          {"label": "Blanc", "value": "white"}
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Affichage LCD"
      },
      {
        "type": "toggle",
        "messageKey": "ShowInactiveSegments",
        "defaultValue": false,
        "label": "Afficher les segments inactifs"
      },
      {
        "type": "toggle",
        "messageKey": "ShowSeconds",
        "defaultValue": true,
        "label": "Afficher les secondes"
      },
      {
        "type": "toggle",
        "messageKey": "BlinkColon",
        "defaultValue": true,
        "label": "Faire clignoter les :"
      }
    ]
  },
  {
    "type": "text",
    "defaultValue": "Les chiffres utilisent des images LCD avec un léger effet de réflexion. Les segments inactifs sont masqués par défaut et peuvent être réactivés ici."
  },
  {
    "type": "submit",
    "defaultValue": "Enregistrer"
  }
];
