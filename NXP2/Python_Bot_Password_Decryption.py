import serial
import time
import random

def connect_serial(port, baudrate):
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        return ser
    except Exception as e:
        print(f"Error connecting to serial port: {e}")
        return None

def generate_numeric_password(length):
    return ''.join(random.choices('0123456789', k=length))

def save_password(password):
    with open('correct_password.txt', 'w') as f:
        f.write(password)

def main():
    ser = connect_serial('COM11', 115200)  # Adjust port and baudrate as necessary
    if ser is None:
        return

    password_length = random.randint(1, 4)
    password = generate_numeric_password(password_length)
    password_found = False  # Flag to indicate if the correct password is found

    try:
        while not password_found:  # Loop indefinitely until the correct password is found
            line = ser.readline().decode('ascii').strip()
            print(f"Received: {line}")  # Print the entire line for debugging
            
            # Check for idle state
            if line == "":
                print("Device is idle, sending carriage return...")
                ser.write(('\r').encode('ascii'))  # Send carriage return to wake the device
                time.sleep(0.5)  # Short delay to avoid flooding the device
                continue
            
            # Check if the line indicates the prompt for entering a password
            if "Enter password" in line:
                print(f"Trying password: {password}")
                ser.write((password + '\r').encode('ascii'))
                time.sleep(1)  # Wait for a response

                # Read the next line(s) for the response
                response = ser.readline().decode('ascii').strip()
                print(f"Response: {response}")

                # Check if the response indicates incorrect or correct password
                if "incorrect" in response.lower():
                    print("Password incorrect.")
                    # Generate a new password for the next attempt
                    password_length = 4
                    password = generate_numeric_password(password_length)
                elif "correct" in response.lower():
                    print("Password accepted!")
                    save_password(password)  # Save the correct password
                    print(f"Correct password saved: {password}")  # Print the correct password
                    password_found = True  # Set the flag to True

    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        ser.close()

if __name__ == "__main__":
    main()