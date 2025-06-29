import json

with open("settings_old.json", 'r') as f:
    data = json.load(f)

    data["default_padding"]= {
                        "left": data["default_padding"][0],
                        "right": data["default_padding"][1],
                        "top": data["default_padding"][2],
                        "bottom": data["default_padding"][3]
            }

    for profile in data["profiles"]:
        for i, display in enumerate(profile["displays"]):
            temporary = {
                "padding": {
                        "left": display["padding"][0],
                        "right": display["padding"][1],
                        "top": display["padding"][2],
                        "bottom": display["padding"][3]
                    },
                    "rotation": {
                        "left": display["rotation"][0],
                        "right": display["rotation"][1],
                        "top": display["rotation"][2],
                        "bottom": display["rotation"][3]
                    }
            }
            profile["displays"][i] = temporary
    
    with open("settings_new.json", 'w') as f:
        json.dump(data, f)

