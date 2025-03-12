use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::mpsc::{Receiver, Sender, channel};
use std::sync::{Arc, Mutex};
use std::thread;

// Struktura User eprezentująca użytkownika czatu
struct User {
    id: usize,
    name: String,
    stream: TcpStream,
}

impl User {
    fn new(id: usize, name: String, stream: TcpStream) -> Self {
        User { id, name, stream }
    }

    fn send_message(&mut self, message: &str) -> std::io::Result<()> {
        self.stream.write_all(format!("{}\n", message).as_bytes())
    }
}

// Struktura Messae reprezentująca wiadomość
struct Message {
    sender_id: usize,
    content: String,
}

// Struktura Serve zarządzająca całym serwerem czatu
struct Server {
    users: Arc<Mutex<Vec<User>>>,
    message_tx: Sender<Message>,
    message_rx: Arc<Mutex<Receiver<Message>>>,
    next_user_id: Arc<Mutex<usize>>,
}

impl Server {
    fn new() -> Self {
        let (tx, rx) = channel::<Message>();
        Server {
            users: Arc::new(Mutex::new(Vec::new())),
            message_tx: tx,
            message_rx: Arc::new(Mutex::new(rx)),
            next_user_id: Arc::new(Mutex::new(0)),
        }
    }

    fn start(&self, address: &str) -> std::io::Result<()> {
        // Uruchamiamy wątk do rozsyłania wiadomości
        self.start_message_broadcaster();

        // Tworzymy nasłuciwacz TCP na podanym adresie
        let listener = TcpListener::bind(address)?;
        println!("Serwer uruchomiony na {}", address);

        // Główna pętla akeptująca nowe połączenia
        for stream in listener.incoming() {
            match stream {
                Ok(stream) => {
                    println!("Nowe połączenie: {}", stream.peer_addr()?);

                    // Obsłuż nowego kienta
                    self.handle_new_client(stream);
                }
                Err(e) => {
                    println!("Błąd podczas akceptacji połączenia: {}", e);
                }
            }
        }

        Ok(())
    }

    fn start_message_broadcaster(&self) {
        let users = Arc::clone(&self.users);
        let message_rx = Arc::clone(&self.message_rx);

        thread::spawn(move || {
            let rx = message_rx.lock().unwrap();
            for message in rx.iter() {
                let mut users_lock = users.lock().unwrap();

                // Lista identyfiktorów użytkowników do usunięcia
                let mut users_to_remove = Vec::new();

                for (i, user) in users_lock.iter_mut().enumerate() {
                    // Nie wysyłaj wiaomości do nadawcy
                    if user.id != message.sender_id {
                        match user.send_message(&message.content) {
                            Ok(_) => {}
                            Err(_) => {
                                println!(
                                    "Nie można napisać do użytkownika {}, zostanie usunięty",
                                    user.name
                                );
                                users_to_remove.push(i);
                            }
                        }
                    }
                }

                // Usuń użytkownikw, którzy nie odpowiadają
                for i in users_to_remove.iter().rev() {
                    let removed = users_lock.remove(*i);
                    println!("Usunięto użytkownika {} z listy", removed.name);
                }

                println!("Aktualnie podłączonych użytkowników: {}", users_lock.len());
            }
        });
    }

    fn handle_new_client(&self, mut stream: TcpStream) {
        // Klonujemy potrzbne referencje dla nowego wątku
        let tx_clone = self.message_tx.clone();
        let users_clone = Arc::clone(&self.users);
        let next_id_clone = Arc::clone(&self.next_user_id);

        thread::spawn(move || {
            let mut name_buffer = [0; 256];
            let bytes_read = match stream.read(&mut name_buffer) {
                Ok(n) => n,
                Err(_) => {
                    println!("Błąd podczas czytania imienia użytkownika");
                    return;
                }
            };

            let name = String::from_utf8_lossy(&name_buffer[..bytes_read])
                .trim()
                .to_string();

            let id = {
                let mut id_lock = next_id_clone.lock().unwrap();
                let id = *id_lock;
                *id_lock += 1;
                id
            };

            let user_stream = match stream.try_clone() {
                Ok(s) => s,
                Err(_) => {
                    println!("Nie udało się sklonować strumienia");
                    return;
                }
            };

            let user = User::new(id, name.clone(), user_stream);

            let join_message = format!("Użytkownik {} dołączył do czatu!", name);
            println!("{}", join_message);

            let welcome = format!("Witaj w czacie, {}! Twój identyfikator: {}\n", name, id);
            stream.write_all(welcome.as_bytes()).unwrap();

            {
                let mut users_lock = users_clone.lock().unwrap();

                for user in users_lock.iter_mut() {
                    if user.id != id {
                        let _ = user.send_message(&join_message);
                    }
                }

                users_lock.push(user);
                println!("Aktualnie podłączonych użytkowników: {}", users_lock.len());
            }

            let mut buffer = [0; 1024];
            loop {
                match stream.read(&mut buffer) {
                    Ok(0) => {
                        println!("Użytkownik {} rozłączył się", name);
                        break;
                    }
                    Ok(bytes_read) => {
                        let msg = String::from_utf8_lossy(&buffer[..bytes_read]).to_string();
                        let trimmed_msg = msg.trim();

                        if trimmed_msg == "exit" {
                            println!("Użytkownik {} opuścił czat", name);
                            break;
                        }

                        println!("Otrzymano od {}: {}", name, trimmed_msg);

                        if let Err(_) = tx_clone.send(Message {
                            sender_id: id,
                            content: format!("{}: {}", name, trimmed_msg),
                        }) {
                            println!("Błąd podczas wysyłania wiadomości do kanału");
                            break;
                        }
                    }
                    Err(_) => {
                        println!(
                            "Błąd podczas odczytywania ze strumienia użytkownika {}",
                            name
                        );
                        break;
                    }
                }
            }

            let mut users_lock = users_clone.lock().unwrap();
            let position = users_lock.iter().position(|u| u.id == id);
            if let Some(pos) = position {
                users_lock.remove(pos);
                println!(
                    "Usunięto użytkownika {} z listy, pozostało {} użytkowników",
                    name,
                    users_lock.len()
                );

                let leave_message = format!("Użytkownik {} opuścił czat", name);
                for user in users_lock.iter_mut() {
                    let _ = user.send_message(&leave_message);
                }
            }
        });
    }
}

fn main() -> std::io::Result<()> {
    let server = Server::new();
    server.start("127.0.0.1:7878")
}
