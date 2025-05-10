from pathlib import Path
import json
from typing import List

class WifiDeet:
    def __init__(self, ssid, password=None):
        self.ssid = ssid
        self.password = password
    
    @classmethod
    def from_bytes(cls, bytes: bytes):
        # First byte indicates if it has a password
        # Second byte is the length of the ssid
        # Next N bytes is the ssid
        # Optionally append another byte for the password length
        # Then M bytes for the password itself
        idx = 0

        has_password = bytes[idx] == 1
        idx +=1

        ssid_len = bytes[idx]
        idx +=1 

        ssid = bytes[idx:ssid_len+idx].decode('utf-8')
        idx += ssid_len
        assert len(ssid) == ssid_len

        if not has_password:
            return cls(ssid, None)
        password_len = bytes[idx]
        idx += 1

        password = bytes[idx:password_len+idx].decode('utf-8')
        idx += password_len
        assert len(password) == password_len
        return cls(ssid, password)
    
    def to_bytes(self) -> bytes:
        result = b""
        if self.password is None:
            result += b"\x00"
        else:
            result += b"\x01"
        
        ssid = self.ssid.encode("utf-8")
        result += len(ssid).to_bytes(1)
        result += ssid

        if self.password is not None:
            password = self.password.encode("utf-8")
            result += len(password).to_bytes(1)
            result += password
        return result

    @classmethod
    def from_dict(cls, data: dict):
        return cls(data["ssid"], data.get("password", None))
    
    def __str__(self):
        return f"{self.ssid}"


def read_wifi_deets(json_file: Path):
    """
    Reads the JSON file containing Wi-Fi details and returns the SSID and password.

    :param json_file: Path to the JSON file
    :return: Tuple containing SSID and password
    """
    result = []
    with open(json_file, 'r') as file:
        data = json.load(file)
        for deets in data:
            result.append(WifiDeet.from_dict(deets))
    return result

def write_wifi_deets(binary_file: Path, deets_list: List[WifiDeet]):
    with binary_file.open("wb") as f:
        deets_count = len(deets_list)
        f.write(deets_count.to_bytes(1))

        for deet in deets_list:
            print(deet)
            f.write(deet.to_bytes())


if __name__ == "__main__":
    # Define the path to the JSON file
    json_file_path = Path(__file__).parent / "wifi_deets.json"
    binary_file_path = Path(__file__).parent / "wifi_deets.bin"

    # Check if the JSON file exists
    if not json_file_path.exists():
        print(f"Error: The file {json_file_path} does not exist.")
        print("Generating one from a template")
        with json_file_path.open("w") as f:
            json.dump([
                {
                    "ssid": "my ssid",
                    "password": "password"
                },
                {
                    "ssid": "other network",
                    "password": "super secret"
                }
            ], f, indent=4)
        exit(1)

    deets = read_wifi_deets(json_file_path)
    write_wifi_deets(binary_file_path, deets)
