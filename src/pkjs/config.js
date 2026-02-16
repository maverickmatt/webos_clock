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
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];