
mod nwa;
mod nwachecker;
use std::convert::TryInto;
use std::process::exit;
use std::net::{ToSocketAddrs};
use crate::nwa::nwa::*;
use crate::nwachecker::nwachecker::*;

fn main() {
    let mut checker : NWAChecker = NWAChecker::new();
    let mut available_commands : Vec<String> = vec![];
    checker.new_check("Address type for localhost",
     "Checking if the emulator listen on the first localhost address. 
     Meaning if it properly bind on the first one, like in case of ipv6. Note that this is not specified but should be the right behavior");
    let mut has_ipv6 = false;
    let addr : Vec<_> = String::from("localhost:65400").to_socket_addrs().expect("Can't resolve address").collect();
    if addr.len() == 2 {
        has_ipv6 = true;
    }
    let mut nwa = NWASyncClient::connect("localhost", 65400);
    let mut nwa : NWASyncClient = match nwa {
        Err(err) => {
            if (has_ipv6) {
                checker.set_passed(false);
                warning("Failed to connect to localhost as ipv6, trying explictly 127.0.0.1");
                NWASyncClient::connect("127.0.0.1", 65400).expect("Failed to connect")
            } else {
                error("Can't connect to an emulator, abord");
                exit(1)
            }
        }
        _ => {checker.set_passed(true);nwa.unwrap()}
    };
    
    checker.new_check("Mandatory EMULATOR_INFO command", "Testing for the firt requiered command");
    let reply = nwa.execute_command("EMULATOR_INFO", None).expect("Error with socket");
    if checker.current_check_expect_ascii_hash(&reply) {
        checker.current_check_add_key_check(&reply, "name", None);
        checker.current_check_add_key_check(&reply, "version", None);
        checker.current_check_add_key_check(&reply, "id", None);
        checker.current_check_add_key_check(&reply, "nwa_version", Some("1.0"));
        let cmd = checker.current_check_add_key_check(&reply, "commands", None);
        if cmd != None {
            let copy = cmd.unwrap().clone();
            available_commands = copy.split(',').map(str::to_string).collect::<Vec<String>>().to_owned();
            println!("\tChecking for mandatory commands");
            checker.current_check_add_subcheck("Mandatory command EMULATOR_INFO", "Plop", available_commands.contains(&String::from("EMULATOR_INFO")));
            checker.current_check_add_subcheck("Mandatory command EMULATION_STATUS", "Plop", available_commands.contains(&String::from("EMULATION_STATUS")));
            checker.current_check_add_subcheck("Mandatory command CORES_LIST", "Plop", available_commands.contains(&String::from("CORES_LIST")));
            checker.current_check_add_subcheck("Mandatory command CORE_INFO", "Plop", available_commands.contains(&String::from("CORE_INFO")));
            checker.current_check_add_subcheck("Mandatory command CORE_CURRENT_INFO", "Plop", available_commands.contains(&String::from("CORE_CURRENT_INFO")));
            checker.current_check_add_subcheck("Mandatory command MY_NAME_IS", "Plop", available_commands.contains(&String::from("MY_NAME_IS")));
        }
    }
    checker.new_check("Mandatory CORES_LIST command", "Test if the mandatory CORES_LIST command reply");
    let reply = nwa.execute_command("CORES_LIST", None).expect("Error with socket");
    let mut available_platform : String = String::from("PLACEHOLDER");
    let mut available_core : Option<String> = None;
    let cp = &reply;
    match cp {
        EmulatorReply::Ascii(rep_asci) => {
            match rep_asci {
                AsciiReply::Hash(map) => {
                    checker.current_check_add_key_check(&reply, "name", None);
                    if map.contains_key("name") {
                        available_core = Some(map.get("name").unwrap().clone());
                    }
                    checker.current_check_add_key_check(&reply, "platform", None);
                    if map.contains_key("platform") {
                        available_platform = map.get("platform").unwrap().clone();
                    }
                    checker.set_passed(true);
                },
                AsciiReply::ListHash(list_map) => {
                    for map in list_map {
                        checker.current_check_add_key_check(&reply, "name", None);
                        checker.current_check_add_key_check(&reply, "platform", None);
                        if map.contains_key("platform") && available_platform == "PLACEHOLDER" {
                            available_platform = map.get("platform").unwrap().clone();
                        }
                        if map.contains_key("name") && available_core == None {
                            available_core = Some(map.get("core").unwrap().clone());
                        }
                        checker.set_passed(true);
                    }
                }
                _ => {error("Replied with an invalid Ascii reply")}
            }
        },
        _ => {error(format!("Did not receive an ascii reply : {:?}", cp).as_str())}
    };
    if available_platform != "PLACEHOLDER" {
        checker.new_check("Command CORES_LIST filter capability", 
        "Test if the CORES_LIST command reply correctly with a given platform taken from the previous request");
        let reply = nwa.execute_command("CORES_LIST", Some(&available_platform)).expect("Error with socket");
        let cp = &reply;
        match cp {
            EmulatorReply::Ascii(rep_asci) => {
                match rep_asci {
                    AsciiReply::Hash(map) => {
                        checker.current_check_add_key_check(&reply, "name", None);
                        checker.current_check_add_key_check(&reply, "platform", None);
                        if map.contains_key("platform") {
                            available_platform = map.get("platform").unwrap().clone();
                        }
                        checker.set_passed(true);
                    },
                    AsciiReply::ListHash(list_map) => {
                        for map in list_map {
                            checker.current_check_add_key_check(&reply, "name", None);
                            checker.current_check_add_key_check(&reply, "platform", None);
                            if map.contains_key("platform") && available_platform == "PLACEHOLDER" {
                                available_platform = map.get("platform").unwrap().clone();
                            }
                        }
                        checker.set_passed(true);
                    }
                    _ => {error("Replied with an invalid Ascii reply")}
                }
            },
            _ => {error(format!("Did not receive an ascii reply : {:?}", cp).as_str())}
        };
    }
    checker.new_check("Command CORES_LIST with invalid platform",
                    "Test the mandaroty CORES_LIST command with an invalid plaform");
    let reply = nwa.execute_command("CORES_LIST", Some("BLABLACORE")).expect("Error with socket");
    checker.current_check_expect_ascii_ok(&reply);

    checker.new_check("Checking Mandatory EMULATION_STATUS command", "Test the mandatory EMULATION_STATUS command");
    let reply = nwa.execute_command("EMULATION_STATUS", None).expect("Error with socket");
    checker.current_check_expect_ascii_hash(&reply);
    let state =  checker.current_check_add_key_check(&reply, "state", None);
    if state != None {
        let state = state.unwrap();
        if state == "running" || state == "paused" {
            checker.current_check_add_key_check(&reply, "game", None);
        }
    }
    if let Some(core) = available_core {
        checker.new_check("Mandatory CORE_INFO", "Testing the mandatory CORE_INFO command");
        let reply = nwa.execute_command("CORE_INFO", Some(core.as_str())).expect("Error with socket");
        checker.current_check_expect_ascii_hash(&reply);
        checker.current_check_add_key_check(&reply, "platform", Some(&available_platform));
        checker.current_check_add_key_check(&reply, "name", Some(core.as_str()));
    }
    checker.new_check("Mandatory CORE_INFO with invalid core name", "Test if the CORE_INFo command return properly an error when given a uncorrect core name");
    let reply = nwa.execute_command("CORE_INFO", Some("invalid_core_name")).expect("Error with socket");
    checker.current_check_expect_error(&reply, nwa::nwa::ErrorKind::InvalidArgument);
    checker.new_check("Checking Mandatory MY_NAME_IS command", "Test the MY_NAME_IS command, check if it returns the id");
    let reply = nwa.execute_command("MY_NAME_IS", Some("NWA validator")).expect("Error with socket");
    checker.current_check_expect_ascii_hash(&reply);
    checker.current_check_add_key_check(&reply, "name", Some("NWA validator"));
    checker.new_check("Unexpected binary block", 
    "Test how the emulator react when giving a binary block when expecting a command, we expect a protocol error and a closed connection");
    let mut data : Vec<u8> = vec![0;3];
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    nwa.send_data(data);
    let reply = nwa.get_reply().expect("Error with socket");
    checker.current_check_expect_error(&reply, nwa::nwa::ErrorKind::ProtocolError);
    let reply = nwa.execute_command("EMULATOR_INFO", None);
    let mut p = false;
    if let Err(err) = reply {
        if err.kind() == std::io::ErrorKind::ConnectionAborted {
            p = true;
        }
    } else {
        nwa.close()
    }
    checker.current_check_add_subcheck("The emulator closed the connection", "After a protocol error the emulator should close the connection", p);
    nwa.reconnected();
    let reply = nwa.execute_command("MY_NAME_IS", Some("NWA validator")).expect("Error with socket");
    let reply = nwa.execute_command("EMULATION_STATUS", None).expect("Socket error");
    let mut state = String::from("no_game");
    match reply {
        EmulatorReply::Ascii(ascii_reply) => {
            match ascii_reply {
                AsciiReply::Hash(hash) => {
                    state = hash.get("state").unwrap().to_string();
                }
                _ => {}
            }
        }
        _ => {}
    }  
    if available_commands.contains(&String::from("CORE_MEMORIES")) {
        let mut available_memory_name = String::from("PLACEHOLDER");
        let mut available_memory_size : u32 = 0;
        checker.new_check("Checking CORE_MEMORIES command", "Checking if CORE_MEMORIES behave correctly");
        let reply = nwa.execute_command("CORE_MEMORIES", None).expect("Error with socket");
        let cp = &reply;
        match cp {
            EmulatorReply::Ascii(rep_asci) => {
                if state == "no_game" {
                    warning("No game is running, CORE_MEMORIES should probably not return something usefull");
                }
                match rep_asci {
                    AsciiReply::Hash(map) => {
                        checker.set_passed(true);
                        checker.current_check_add_key_check(&reply, "name", None);
                        checker.current_check_add_key_check(&reply, "access", None);
                        if map.contains_key("name") && map.contains_key("access") && map.get("access").unwrap().contains("r") {
                            available_memory_name = map.get("name").unwrap().clone();
                            if map.contains_key("size") {
                                available_memory_size = map.get("size").unwrap().parse::<u32>().unwrap();
                            }
                        }
                    },
                    AsciiReply::ListHash(list_map) => {
                        checker.set_passed(true);
                        for map in list_map {
                            checker.current_check_add_key_check(&reply, "name", None);
                            checker.current_check_add_key_check(&reply, "platform", None);
                            if available_memory_name == "PLACEHOLDER" && 
                               map.contains_key("name") && map.contains_key("access") && map.get("access").unwrap().contains("r") {
                                available_memory_name = map.get("name").unwrap().clone();
                                if map.contains_key("size") {
                                    available_memory_size = map.get("size").unwrap().parse::<u32>().unwrap();
                                }
                            }
                        }
                    }
                    _ => {error("Replied with an invalid Ascii reply")}
                }
            },
            EmulatorReply::Error(err) => { checker.set_passed(true); }
        _ => {error(format!("Did not receive an ascii or error reply : {:?}", cp).as_str())}
        }
        if available_memory_name != "PLACEHOLDER" {
            checker.new_check("Basic CORE_READ", "Reading 5 bytes from one readable memory");
            let reply = nwa.execute_command("CORE_READ", Some(format!("{:};0;5", available_memory_name).as_str())).expect("Error with socket");
            checker.current_check_expect_binary_block(&reply, 5);
            checker.new_check("Basic CORE_READ, whole memory", "Reading a whole readable memory");
            let reply = nwa.execute_command("CORE_READ", Some(format!("{:}", available_memory_name).as_str())).expect("Error with socket");
            checker.current_check_expect_binary_block(&reply, available_memory_size.try_into().unwrap());
            checker.new_check("Basic CORE_READ, whole memory starting at 20", "Reading whole memory minus 20");
            let reply = nwa.execute_command("CORE_READ", Some(format!("{:};20", available_memory_name).as_str())).expect("Error with socket");
            checker.current_check_expect_binary_block(&reply, (available_memory_size - 20).try_into().unwrap());
            checker.new_check("Basic CORE_READ, multiple offset", "Reading multiple offset of a memory");
            let reply = nwa.execute_command("CORE_READ", Some(format!("{:};8;5;25;4;30;1", available_memory_name).as_str())).expect("Error with socket");
            checker.current_check_expect_binary_block(&reply, 10);
            checker.new_check("Basic CORE_READ, multiple offset without size", "Reading multiple offset but without specify the size of the last one");
            let reply = nwa.execute_command("CORE_READ", Some(format!("{:};20;2;{:}", available_memory_name, available_memory_size - 2).as_str())).expect("Error with socket");
            checker.current_check_expect_binary_block(&reply, 4);

        }
    }
    checker.repport();
}
