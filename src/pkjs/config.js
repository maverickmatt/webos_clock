module.exports = [
  // {
  //   "type": "heading",
  //   "defaultValue": "App Configuration"
  // },
  {
    "type": "text",
    "defaultValue": "webOS Flip Clock settings"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "select",
        "messageKey": "DateFmt",
        "defaultValue": "0",
        "label": "Date format",
        "options": [
          { 
            "label": "mm/dd/yy",
            "value": "0" 
          },
          { 
            "label": "dd.mm.yy",
            "value": "1" 
          }
        ]
      },
      {
        "type": "toggle",
        "capabilities": [ "DISPLAY_144x168" ],
        "messageKey": "LargeFont",
        "label": "Large font",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "capabilities": [ "PLATFORM_CHALK" ],
        "messageKey": "LargeFont",
        "label": "Large font",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];