use rustyline::error::ReadlineError;
use rustyline::DefaultEditor;
use serde_json::Value;

#[tokio::main]
async fn main() {
    println!("============================================");
    println!(" RogueCities AI Rust REPL (SpacetimeDB env) ");
    println!("============================================");
    
    let mut rl = DefaultEditor::new().unwrap();
    
    loop {
        let readline = rl.readline("ai> ");
        match readline {
            Ok(line) => {
                let line = line.trim();
                if line.is_empty() {
                    continue;
                }
                
                let _ = rl.add_history_entry(line);
                
                if line == "exit" || line == "quit" {
                    break;
                }
                
                // TODO: Implement actual command routing and SpacetimeDB connection.
                println!("Command received: {}", line);
                
                if line.starts_with("ask ") {
                    let prompt = &line[4..];
                    println!("Sending prompt to Ollama: {}", prompt);
                    // Mock implementation for now
                    let response = query_ollama(prompt).await;
                    println!("AI Response: {}", response);
                } else {
                    println!("Unknown command. Type 'ask <prompt>' to query AI, or 'exit' to quit.");
                }
            },
            Err(ReadlineError::Interrupted) => {
                println!("CTRL-C");
                break;
            },
            Err(ReadlineError::Eof) => {
                println!("CTRL-D");
                break;
            },
            Err(err) => {
                println!("Error: {:?}", err);
                break;
            }
        }
    }
}

async fn query_ollama(prompt: &str) -> String {
    // Placeholder logic for calling Ollama
    // You would use reqwest here to post to http://localhost:11434/api/generate
    format!("(Mock response for: {})", prompt)
}
