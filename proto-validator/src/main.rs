
mod nwa;
use std::io::ErrorKind;
use std::process::exit;
use std::net::{ToSocketAddrs};
use crate::nwa::nwa::*;
use colored::*;

struct NWACheck {
    name : String,
    description : String,
    passed : bool,
    sub_checks : Vec<NWACheck>
}

struct NWAChecker {
    all_checks : Vec<NWACheck>
}

impl NWAChecker {
    pub fn new () -> NWAChecker {
        NWAChecker {
            all_checks : vec![]
        }
    }
    pub fn new_check(&mut self, short_description : &str, description : &str) {
        self.all_checks.push(NWACheck {
            name : short_description.to_string(),
            description : description.to_string(),
            passed : false,
            sub_checks : vec![]
        });
        println!("* Checking : {}", short_description);
    }
    pub fn set_passed(&mut self, p : bool) {
        self.all_checks.last_mut().unwrap().passed = p;
    }
    pub fn current_check_expect_ascii_hash(&mut self, reply : &EmulatorReply) -> bool {
        let current_check = self.all_checks.last_mut().unwrap();
        match reply {
            EmulatorReply::Ascii(ascii_rep) => {
                match ascii_rep {
                    AsciiReply::Hash(map) => {
                        current_check.passed = true;
                    }
                    _ => {current_check.passed = false;}
                }
            }
            _ => {current_check.passed = false}
        }
        if current_check.passed == false {
            error(format!("Did not receive an ascii map reply : {:?}", reply).as_str());
        }
        current_check.passed
    }
    pub fn current_check_expect_ascii_ok(&mut self, reply :& EmulatorReply) -> bool {
        let current_check = self.all_checks.last_mut().unwrap();
        match reply {
            EmulatorReply::Ascii(ascii_rep) => {
                match ascii_rep {
                    AsciiReply::Ok => {
                        current_check.passed = true;
                    }
                    _ => {current_check.passed = false;}
                }
            }
            _ => {current_check.passed = false}
        }
        if current_check.passed == false {
            error(format!("Did not receive an ascii ok (empty success) reply : {:?}", reply).as_str());
        }
        current_check.passed
    }
    pub fn current_check_expect_error(&mut self, reply :& EmulatorReply, kind : nwa::nwa::ErrorKind) -> bool {
        let current_check = self.all_checks.last_mut().unwrap();
        match reply {
            EmulatorReply::Error(error) => {
                current_check.passed = error.kind == kind;
            }
            _ => {current_check.passed = false}
        }
        if current_check.passed == false {
            error(format!("Did not receive the proper error reply, expected : {:?} - got {:?}", kind, reply).as_str());
        }
        current_check.passed
    }
    pub fn current_check_add_key_check(&mut self, reply : &EmulatorReply, key : &str, value : Option<&str>) -> Option<String> {
        let current_check = self.all_checks.last_mut().unwrap();
        current_check.sub_checks.push({
            NWACheck {
                name : String::from("subcheck"),
                description : String::from("no desc"),
                passed : false,
                sub_checks : vec![]
            }
        });
        let mut current_subcheck = current_check.sub_checks.last_mut().unwrap();
        let mut key_value : Option<String> = None;
        if let EmulatorReply::Ascii(asci_rep) = reply {
            if let AsciiReply::Hash(map) = asci_rep {
                if map.contains_key(key) {
                    key_value = Some(map.get(key).unwrap().clone());
                }
                if value == None {
                    current_subcheck.passed = map.contains_key(key);
                } else {
                    current_subcheck.passed = map.get(key).unwrap() == value.unwrap();
                }
            } else {
                current_subcheck.passed = false;
            }
        }
        expect_true(current_subcheck.passed, format!("\tChecking for mandatory field <{:?}> : ", key).as_str());
        return key_value;
    }
}

fn expect_ok(reply : EmulatorReply) {
    match reply {
        EmulatorReply::Ascii(AsciiReply::Ok) => {println!("{}", "Ok".green())},
        _ => {println!("{} Expected an Ok reply", "Fail".red())}
    }
}

fn expect_ascii(reply : EmulatorReply, key : Option<&str>, value : Option<&str>) {

}

fn expect_true(b : bool, msg : &str) {
    print!("{}", msg);
    if b {
        print!("{}", "ok".green())
    } else {
        print!("{}", "ko".red())
    }
    println!();
}

fn new_check(msg : &str) {
    println!("* Checking : {}", msg);
}

fn warning(msg : &str) {
    println!("\t{} {}", "Warning".yellow(), msg)
}

fn error(msg : &str) {
    println!("\t{} {}", "Error".red(), msg)
}
fn main() {
    let mut checker : NWAChecker = NWAChecker::new();
    checker.new_check("Address type for localhost",
     "Checking if the emulator listen on the first localhost address. 
     Meaning if it properly bind on the first one, like in case of ipv6");
    let mut has_ipv6 = false;
    let addr : Vec<_> = String::from("localhost:65400").to_socket_addrs().expect("Can't resolve address").collect();
    if addr.len() == 2 {
        has_ipv6 = true;
    }
    let mut nwa = NWASyncClient::connect("localhost", 65400);
    let mut nwa : NWASyncClient = match nwa {
        Err(err) => {
            if (has_ipv6) {
                warning("Failed to connect to localhost as ipv6, trying explictly 127.0.0.1");
                NWASyncClient::connect("127.0.0.1", 65400).expect("Failed to connect")
            } else {
                error("Can't connect to an emulator, abord");
                exit(1)
            }
        }
        _ => {nwa.unwrap()}
    };
    checker.set_passed(true);
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
            let commands : Vec<&str> = copy.split(',').collect();
            println!("\tChecking for mandatory commands");
            expect_true(commands.contains(&"EMULATOR_INFO"), "\tChecking for mandatory command EMULATOR_INFO : ");
            expect_true(commands.contains(&"EMULATION_STATUS"), "\tChecking for mandatory command EMULATION_STATUS : ");
            expect_true(commands.contains(&"CORES_LIST"), "\tChecking for mandatory command CORES_LIST : ");
            expect_true(commands.contains(&"CORE_INFO"), "\tChecking for mandatory command CORE_INFO : ");
            expect_true(commands.contains(&"CORE_CURRENT_INFO"), "\tChecking for mandatory command CORE_CURRENT_INFO : ");
        }
    }
    checker.new_check("Mandatory CORES_LIST command", "Test if the mandatory CORES_LIST command reply");
    let reply = nwa.execute_command("CORES_LIST", None).expect("Error with socket");
    let mut available_platform : String = String::from("PLACEHOLDER");
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
                },
                AsciiReply::ListHash(list_map) => {
                    for map in list_map {
                        checker.current_check_add_key_check(&reply, "name", None);
                        checker.current_check_add_key_check(&reply, "platform", None);
                        if map.contains_key("platform") && available_platform == "PLACEHOLDER" {
                            available_platform = map.get("platform").unwrap().clone();
                        }
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
                    },
                    AsciiReply::ListHash(list_map) => {
                        for map in list_map {
                            checker.current_check_add_key_check(&reply, "name", None);
                            checker.current_check_add_key_check(&reply, "platform", None);
                            if map.contains_key("platform") && available_platform == "PLACEHOLDER" {
                                available_platform = map.get("platform").unwrap().clone();
                            }
                        }
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
    let state =  checker.current_check_add_key_check(&reply, "state", None);
    if state != None {
        let state = state.unwrap();
        if state == "running" || state == "paused" {
            checker.current_check_add_key_check(&reply, "game", None);
        }
    }
    if available_platform != "PLACEHOLDER" {
        checker.new_check("Mandatory CORE_INFO", "Testing the mandatory CORE_INFO command");
        let reply = nwa.execute_command("CORE_INFO", Some(&available_platform)).expect("Error with socket");
        checker.current_check_add_key_check(&reply, "platform", Some(&available_platform));
        checker.current_check_add_key_check(&reply, "name", None);
    }

    checker.new_check("Unexpected binary block", 
    "Test how the emulator react when giving a binary block when expecting a command");
    let mut data : Vec<u8> = vec![0;3];
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    nwa.send_data(data);
    let reply = nwa.get_reply().expect("Error with socket");
    checker.current_check_expect_error(&reply, nwa::nwa::ErrorKind::ProtocolError);
}
