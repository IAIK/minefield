import paramiko
import time
import sys
import os
import subprocess

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
    "mobile-lab07" : (True, 17),
    "mobile-lab05" : (True, 27),
    "mobile-lab06" : (True, 27),
    "mobile-lab10" : (True, 27),
    "uarch-lab12" : (False, 5),
    "lab-machine" : (True, 17),
  }

  if remote not in pdu_pin_map:
    print("could not find reset pin")
    exit(0)

  from gpiozero import LED
  from gpiozero.pins.pigpio import PiGPIOFactory

  retries = 0

  while True:
    print(f"\r> connecting to pdu    (try {retries:3d}) ",end='')
    try:
      rpi = create_ssh(raspberry_pdu, 10)
      #factory = PiGPIOFactory(host=raspberry_pdu)
      #gpio = LED(17, pin_factory=factory, initial_value=True)
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

def remote_connect(remote, pdu, reset_pin):
  retries = 0

  while True:
    print(f"\r> connecting to remote (try {retries:3d}) ",end='')
    try:
      ssh = create_ssh(remote, timeout=5)
      print(f"- connection established")
      time.sleep(1)
      return (ssh, reset_pin)
    
    except KeyboardInterrupt:
      raise
    
    except:
      # could not connect to remote:
      pass
    # retry
    time.sleep(1)

    pdu_trigger = (retries % 100) == 0

    retries += 1

    if not pdu_trigger:
      # already restarted remote just wait
      continue

    # reset remote
    while not pdu_restart_remote(reset_pin):
      # could not set pin of pdu
      # reconnect to pdu
      print("")
      reset_pin = pdu_get_reset_pin(remote, pdu)
      time.sleep(1)


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

def remote_undervolt(ssh, file, voltage_offset, frequency, core, rounds, iterations, verbose, timeout, trap_factor, trap_init):
  vb = "--verbose" if verbose else ""

  cmd = ""
  cmd += "cd ~/analyse-instructions/ &&"
  cmd += f"sudo python3 analyse.py undervolt --input='{file}' --vo_start={voltage_offset} --freq_start={frequency} --core={core} --rounds={rounds} --iterations={iterations} {vb} --trap_factor={trap_factor} --trap_init={trap_init}"

  success, output = remote_invoke_command(ssh, cmd, show=True, interactive=True, timeout=timeout)

  faulted = "faulted" in output

  return success, faulted

def remote_run(files, voltage_offset, frequency, core, rounds, iterations, skip_until, verbose, remote, pdu, timeout, retry_count, prev_connection, trap_factor, trap_init):
  # connect to the remote

  if prev_connection is None:
    reset_pin = pdu_get_reset_pin(remote, pdu)
    ssh, reset_pin = remote_connect(remote, pdu, reset_pin)
    prev_connection = ssh, reset_pin
  else:
    ssh, reset_pin = prev_connection

  faulted_files = []
  skipped_files = []

  # ron for each file
  try:
    for file in files:
      if file == "":
        continue

      # skip until start after
      if skip_until is not None and skip_until in file:
        skip_until = None
      elif skip_until is not None:
        continue
      
      for _ in range(retry_count):
        success, faulted = remote_undervolt(ssh, file, voltage_offset, frequency, core, rounds, iterations, verbose, timeout, trap_factor, trap_init)
        if success:
          break
        print("T timed out")
        ssh, reset_pin = remote_connect(remote, pdu, reset_pin)
        prev_connection = ssh, reset_pin
      else:
        skipped_files.append(file)
        print(f"> skipped '{file}' after to many retries")
  
      if faulted:
        faulted_files.append(file)

  except:
    ssh.close()
    raise

  return prev_connection, faulted_files, skipped_files
