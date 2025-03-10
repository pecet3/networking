use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use thread_pool::ThreadPool;

mod thread_pool;

fn handle_client(mut stream: TcpStream) {
    let mut buffer = [0; 4096];
    loop {
        let bytes_read = stream.read(&mut buffer).unwrap();
        if bytes_read == 0 {
            break;
        }
        stream.write(&buffer[..bytes_read]).unwrap();
    }
}

fn main() -> std::io::Result<()> {
    let listener = TcpListener::bind("127.0.0.1:90")?;
    println!("Server listening on port 8080");

    let pool = ThreadPool::new(4); // Tworzymy pulę 4 wątków

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                pool.execute(|| {
                    handle_client(stream);
                });
            }
            Err(e) => {
                eprintln!("Failed to accept connection: {}", e);
            }
        }
    }
    Ok(())
}
