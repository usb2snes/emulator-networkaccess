
pub mod nwachecker {

    use crate::nwa::nwa::*;
    use colored::*;
struct NWACheck {
    name : String,
    description : String,
    passed : bool,
    sub_checks : Vec<NWACheck>
}

pub struct NWAChecker {
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
    pub fn end_of_check(&mut self) {
        let current_check = self.all_checks.last().unwrap();
        if current_check.passed == false {
            
        }
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
    pub fn current_check_expect_binary_block(&mut self, reply :& EmulatorReply, size : usize) -> bool {
        let current_check = self.all_checks.last_mut().unwrap();
        current_check.passed = false;
        match reply {
            EmulatorReply::Binary(data) => {
                if data.len() == size {
                    current_check.passed = true;
                }
            }
            _ => {

            }
        }
        if current_check.passed == false {
            error(format!("Did not receive a correct binary block : {:?}", reply).as_str());
        }
        current_check.passed
    }
    pub fn current_check_expect_error(&mut self, reply :& EmulatorReply, kind : ErrorKind) -> bool {
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
                name : String::from(format!("Checking for mandatory key {:}", key)),
                description : String::from("no desc"),
                passed : false,
                sub_checks : vec![]
            }
        });
        let mut current_subcheck = current_check.sub_checks.last_mut().unwrap();
        let mut key_value : Option<String> = None;
        if let EmulatorReply::Ascii(asci_rep) = reply {
            if let AsciiReply::Hash(map) = asci_rep {
                let mut has_key = false;
                if map.contains_key(key) {
                    key_value = Some(map.get(key).unwrap().clone());
                    has_key = true;
                }
                if value == None {
                    current_subcheck.passed = map.contains_key(key);
                } else {
                    if has_key {
                        current_subcheck.passed = map.get(key).unwrap() == value.unwrap();
                    } else {
                        current_subcheck.passed = false;
                    }
                }
            } else {
                current_subcheck.passed = false;
            }
        }
        expect_true(current_subcheck.passed, format!("\tChecking for mandatory field <{:?}> : ", key).as_str());
        return key_value;
    }
    pub fn current_check_add_subcheck(&mut self, name : &str, description : &str, passed : bool) {
        let current_check = self.all_checks.last_mut().unwrap();
        expect_true(passed, format!("\tChecking for {:} : ", name).as_str());
        current_check.sub_checks.push({
            NWACheck {
                name : name.to_string(),
                description : description.to_string(),
                passed : passed,
                sub_checks : vec![]
            }
        });
    }
    pub fn repport(&mut self) {
        let mut passed_count = 0;
        let total_check = self.all_checks.len();
        println!("\n\nRepport for the run:\n");
        for check in &self.all_checks {
            let mut c_passed = true;
            if !check.sub_checks.is_empty() {
                for sub_check in &check.sub_checks {
                    if sub_check.passed == false {
                        c_passed = false;
                    }
                }
            }
            if c_passed && check.passed {
                passed_count += 1;
            } else {
                println!("Check failed : {:}\n\t{:}", check.name, check.description);
                if !check.sub_checks.is_empty() {
                    for sub_check in &check.sub_checks {
                        if sub_check.passed == false {
                            println!("Sub check failed : {:}", sub_check.name)
                        }
                    }    
                }
            }
        }
        println!("Check passed : {:} / {:}", passed_count, total_check);
    }
}

pub fn expect_ok(reply : EmulatorReply) {
    match reply {
        EmulatorReply::Ascii(AsciiReply::Ok) => {println!("{}", "Ok".green())},
        _ => {println!("{} Expected an Ok reply", "Fail".red())}
    }
}


pub fn expect_true(b : bool, msg : &str) {
    print!("{}", msg);
    if b {
        print!("{}", "ok".green())
    } else {
        print!("{}", "ko".red())
    }
    println!();
}

pub fn new_check(msg : &str) {
    println!("* Checking : {}", msg);
}

pub fn warning(msg : &str) {
    println!("\t{} {}", "Warning".yellow(), msg)
}

pub fn error(msg : &str) {
    println!("\t{} {}", "Error".red(), msg)
}
}