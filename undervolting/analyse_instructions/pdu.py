#!/usr/bin/env python3

import paramiko
import sys
import os
import time

def remote_invoke_command(ssh : paramiko.SSHClient, cmd, show=False, interactive=False, timeout=None):
  out_str = ""

  try:
    stdin, ssh_stdout, ssh_stderr = ssh.exec_command(cmd, timeout=timeout)
    stdin.channel.shutdown_write()

    if interactive:
      # capture byte wise
      out_bytes = []
      out_str = ""
      while (x := ssh_stdout.read(1)) != b'':
        out_bytes.append(x)
        out_str = b''.join(out_bytes).decode("utf-8")
      
    else:
      # capture after completion
      out_str = ssh_stdout.read().decode("utf-8")
      err_str = ssh_stderr.read().decode("utf-8")
      if err_str != "":
        print("remote stderr:")
        print(err_str)

  except KeyboardInterrupt:
    raise
  except:
    success = False
  else:
    success = True

  if show:
    print(out_str, end='')

  return (success, out_str)

def create_ssh(host_name, timeout):
  #print(f"searchin for {host_name}")
  conf = paramiko.SSHConfig()
  conf.parse(open(os.path.expanduser('~/.ssh/config')))
  host = conf.lookup(host_name)
  client = paramiko.SSHClient()
  client.load_system_host_keys()
  client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

  proxy = host.get('proxycommand')

  client.connect(
    host['hostname'],
    username=host['user'],
    key_filename=host['identityfile'] if 'keyfile' in host else None,
    sock=paramiko.ProxyCommand(proxy) if proxy is not None else None,
    timeout=timeout,
    banner_timeout=1000,
    port=host['port'] if 'port' in host else None
  )
  
  return client

def pdu_get_reset_pin(remote, raspberry_pdu):
  pdu_pin_map = {
    "mobile-lab09" : (True, 27),
    "mobile-lab07" : (True, 17),
    "lab-machine" : (True, 17),
  }

  if remote not in pdu_pin_map:
    print("could not find reset pin")
    exit(0)

  retries = 0

  while True:
    print(f"\r> connecting to pdu    (try {retries:3d}) ",end='')
    try:
      rpi = create_ssh(raspberry_pdu, 10)
      print(f"- connection established")
      return (rpi, pdu_pin_map[remote])
    except KeyboardInterrupt:
      raise
    except:
      # could not connect to pdu
      pass
    # we can only retry ... we do not have a pdu for the pdu
    time.sleep(10)
    retries += 1

def pdu_restart_remote(reset_pin):
  pdu, (command, pin) = reset_pin

  retries = 0
  while retries < 3:
    try:

      if command == True:
        success1, _ = remote_invoke_command(pdu, f"pigs w {pin} 0")
        time.sleep(1)
        success2, _ = remote_invoke_command(pdu, f"pigs w {pin} 1")
      else:
        success1, _ = remote_invoke_command(pdu, f"pdu-reset {pin}")
        time.sleep(1)
        success2 = True

      if success1 and success2:
        return True
    
    except KeyboardInterrupt:
      raise

    except:
      retries += 1
  
  return False


pdu_restart_remote(pdu_get_reset_pin(sys.argv[1], "pi-pdu"))