import socket
import tkinter as tk
import threading

stop = False

class Gui:
    def __init__(self):
        self.connect_entry = None
        self.connect_label_error = None
        self.room_entry = None
        self.room_label_error = None
        self.gomoku_label = None
        self.list_pola = []
        self.ruch = False
        self.sign = [" X ", " O "]
        self.bg = ["#cc3333", "#33cc33"]
        self.window = self.__create_window()
        self.socket = self.__create_socket()
        self.last_ruch = None
        self.frame_connect = self.__create_frame_connect()
        self.frame_select_room = self.__create_frame_select_room()
        self.frame_gomoku = self.__create_frame_gomoku()
        self.frame_wait = self.__create_frame_wait()
        self.frame_disconnect = self.__create_frame_disconnect()
        self.frame_connect.pack()

    @staticmethod
    def __create_window():
        window = tk.Tk()
        window.title("Gomoku")
        window.geometry('+10+10')  # odległość od górnej i lewej krawędzi
        window.resizable(False, False)
        window.minsize(400, 400)
        window.update_idletasks()
        return window

    @staticmethod
    def __create_socket():
        socket_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        socket_client.settimeout(1)
        return socket_client

    def __create_frame_connect(self):
        frame = tk.Frame(master=self.window)
        label = tk.Label(master=frame, text="Wpisz adres serwera")
        self.connect_entry = tk.Entry(master=frame)
        button = tk.Button(master=frame, text="Połącz", command=self.__connect_to_server)
        self.connect_label_error = tk.Label(master=frame, text="")
        label.pack()
        self.connect_entry.pack()
        button.pack()
        self.connect_label_error.pack()
        return frame

    def __create_frame_select_room(self):
        frame = tk.Frame(master=self.window)
        label = tk.Label(master=frame, text="Podaj numer pokoju")
        self.room_entry = tk.Entry(master=frame)
        button = tk.Button(master=frame, text="Wejdź do pokoju", command=self.__select_room)
        self.room_label_error = tk.Label(master=frame, text="")
        label.pack()
        self.room_entry.pack()
        button.pack()
        self.room_label_error.pack()
        return frame

    def __create_frame_wait(self):
        frame = tk.Frame(master=self.window)
        label = tk.Label(master=frame, text="Oczekiwanie na przeciwnika")
        label.pack()
        return frame

    def __create_frame_gomoku(self):
        frame = tk.Frame(master=self.window)
        self.gomoku_label = tk.Label(master=frame, text="")
        self.gomoku_label.pack()
        frameBoard = tk.Frame(master=frame)
        frameBoard.pack()
        for i in range(15):
            for j in range(15):
                kk = lambda k: (lambda: self.__gomoku_zagrywajacy(k))
                button = tk.Button(master=frameBoard, text="     ", command=kk(i * 15 + j))
                button.grid(row=i, column=j)
                self.list_pola.append(button)
        return frame

    def __create_frame_disconnect(self):
        frame = tk.Frame(master=self.window)
        label = tk.Label(master=frame, text="Utracono połączenie z serwerem")
        label.pack()
        return frame

    def __connect_to_server(self):
        address = self.connect_entry.get()
        print(address)
        try:
            self.socket.connect((address, 1100))
            self.frame_connect.destroy()
            self.frame_select_room.pack()
        except OSError:
            self.connect_label_error.config(text="Błędny adres")

    def __select_room(self):
        t3 = threading.Thread(target=self.__select_room__thread)
        t3.start()

    def __select_room__thread(self):
        try:
            room = int(self.room_entry.get())
            self.socket.send(room.to_bytes(4, 'little', signed=True))
            data = int.from_bytes(self.socket.recv(4), "little", signed=True)
            if data <= 0:
                if data == 0:
                    self.frame_select_room.destroy()
                    self.frame_disconnect.pack()
                elif data == -3:
                    self.room_label_error.config(text="Błąd")
                elif data == -2:
                    self.room_label_error.config(text="Niepoprawny numer pokoju")
                else:
                    self.room_label_error.config(text="Pokój zajęty")
                return
        except ValueError:
            self.room_label_error.config(text="Numer pokoju musi być liczbą całkowitą")
            return
        self.frame_select_room.destroy()
        self.frame_wait.pack()
        while True:
            global stop
            if stop:
                return
            try:
                ruch = int.from_bytes(self.socket.recv(4), "little", signed=True)
                if ruch == 0:
                    self.frame_wait.destroy()
                    self.frame_disconnect.pack()
                    return
                if ruch == 1:
                    self.ruch, self.sign, self.bg = True, self.sign[::-1], self.bg[::-1]
                    self.gomoku_label.config(text="Twój ruch")
                self.socket.send(ruch.to_bytes(4, 'little', signed=True))
                break
            except socket.timeout:
                continue
            except ConnectionResetError:
                self.frame_wait.destroy()
                self.frame_disconnect.pack()
                return
        self.frame_wait.destroy()
        self.frame_gomoku.pack()
        if not self.ruch:
            self.__gomoku_przyjmujacy()

    def __gomoku_zagrywajacy(self, pole):
        if not self.ruch:
            return
        self.socket.send(pole.to_bytes(4, 'little', signed=True))
        pole_ok = 2
        while pole_ok < -1 or pole_ok > 1:
            pole_ok = int.from_bytes(self.socket.recv(4), "little", signed=True)
        if pole_ok == -1:
            return
        self.ruch = False
        if pole_ok == 0:
            self.gomoku_label.config(text="Utracono połączenie z serwerem")
        if self.last_ruch is not None:
            self.last_ruch.config(bg=self.bg[1])
        self.list_pola[pole].config(text=self.sign[0], bg="yellow")
        self.last_ruch = self.list_pola[pole]
        ruch_result = int.from_bytes(self.socket.recv(4), "little", signed=True)
        print(ruch_result)
        if ruch_result > 1:
            self.socket.close()
            if ruch_result == 2:
                self.gomoku_label.config(text="Przegrana")
            elif ruch_result == 3:
                self.gomoku_label.config(text="Remis")
            elif ruch_result == 4:
                self.gomoku_label.config(text="Wygrana")
            else:
                self.gomoku_label.config(text="Wygrana przez poddanie")
        elif ruch_result == 0:
            self.socket.close()
            self.gomoku_label.config(text="Utracono połączenie z serwerem")
        else:
            self.__gomoku_przyjmujacy()

    def __gomoku_przyjmujacy(self):
        t3 = threading.Thread(target=self.__gomoku_przyjmujacy__thread)
        t3.start()

    def __gomoku_przyjmujacy__thread(self):
        self.gomoku_label.config(text="Ruch rywala")
        ruch_rywala = 0
        while True:
            global stop
            if stop:
                return
            try:
                ruch_rywala = int.from_bytes(self.socket.recv(4), "little", signed=True)
                break
            except socket.timeout:
                continue
            except ConnectionResetError:
                self.socket.close()
                self.gomoku_label.config(text="Utracono połączenie z serwerem")
                return
        if ruch_rywala <= 0:
            if ruch_rywala == -2:
                self.gomoku_label.config(text="Wygrana przez poddanie")
            else:
                self.gomoku_label.config(text="Utracono połączenie z serwerem")
            self.socket.close()
            return
        if ruch_rywala > 0:
            ruch_rywala -= 1
        self.ruch = True
        self.list_pola[ruch_rywala].config(text=self.sign[1], bg="yellow")
        if self.last_ruch is not None:
            self.last_ruch.config(bg=self.bg[0])
        self.last_ruch = self.list_pola[ruch_rywala]

        ruch_result = int.from_bytes(self.socket.recv(4), "little", signed=True)
        if ruch_result > 1:
            self.ruch = False
            self.socket.close()
            if ruch_result == 2:
                self.gomoku_label.config(text="Przegrana")
            elif ruch_result == 3:
                self.gomoku_label.config(text="Remis")
            elif ruch_result == 4:
                self.gomoku_label.config(text="Wygrana")
            else:
                self.gomoku_label.config(text="Wygrana przez poddanie")
        elif ruch_result == 0:
            self.socket.close()
            self.gomoku_label.config(text="Utracono połączenie z serwerem")
            self.ruch = False
        else:
            self.gomoku_label.config(text="Twój ruch")


def main():
    gui = Gui()
    gui.window.mainloop()
    global stop
    stop = True

main()