use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::{Arc, Mutex};
use thread_pool::ThreadPool;

mod thread_pool;

#[derive(Debug)]
struct User {
    name: Arc<str>,
}
impl User {
    fn new(name: String) -> Self {
        User {
            name: Arc::from(name),
        }
    }
}
#[derive(Debug)]
struct Server {
    users: Vec<User>,
}

impl Server {
    fn new() -> Self {
        Server { users: Vec::new() }
    }
    fn add_user(&mut self, name: String) {
        self.users.push(User::new(name));
        println!("Added a new user: {:?}", self);
    }
}

fn handle_client(srv: Arc<Mutex<Server>>, mut stream: TcpStream) {
    stream.write(b"provide your Name: ").unwrap_or_default();

    let mut name_buffer = [0; 8];
    let bytes_read = match stream.read(&mut name_buffer) {
        Err(e) => {
            println!("error {}", e);
            return;
        }
        Ok(0) => return,
        Ok(n) => n,
    };
    if bytes_read == 0 {
        return;
    }
    let name = String::from_utf8(name_buffer[..bytes_read].to_vec()).unwrap();

    let mut server = match srv.lock() {
        Err(e) => {
            println!("{}", e);
            return;
        }
        Ok(s) => s,
    };
    server.add_user(name.trim().to_string());

    let mut buffer = [0; 4096];
    let message = format!("{} joined\n\0", name.trim());
    stream
        .write_all(message.trim().as_bytes())
        .unwrap_or_default();

    loop {
        let bytes_read = match stream.read(&mut buffer) {
            Err(e) => {
                println!("{}", e);
                break;
            }
            Ok(0) => continue,
            Ok(n) => n,
        };
        if bytes_read == 0 {
            break;
        }
        match String::from_utf8(buffer[..bytes_read].to_vec()) {
            Ok(msg) => {
                println!("msg {}", msg);
                stream.write(&buffer[..bytes_read]).unwrap_or_default();
            }

            Err(err) => {
                println!("error {}", err);
            }
        }
    }
}

fn main() -> std::io::Result<()> {
    let listener = TcpListener::bind("127.0.0.1:8090")?;
    println!("Server listening");

    let pool = ThreadPool::new(4); // Tworzymy pulę 4 wątków
    let srv = Arc::new(Mutex::new(Server::new()));

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                let srv_clone = srv.clone();
                pool.execute(|| {
                    handle_client(srv_clone, stream);
                });
            }
            Err(e) => {
                eprintln!("Failed to accept connection: {}", e);
            }
        }
    }
    Ok(())
}
