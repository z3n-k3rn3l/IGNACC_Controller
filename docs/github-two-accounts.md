# Two GitHub accounts, one computer, one repo

This is how this project is pushed to **two private GitHub repos owned by two
different accounts**, from one Windows 11 PC. The same setup works on Linux.
Only the file-permission step differs.

The accounts used here:

| Account | Repo | SSH key |
|---|---|---|
| **z3n-k3rn3l** | `z3n-k3rn3l/IGNACC_Controller` | `~/.ssh/id_ed25519` |
| **GeoLCost** | `GeoLCost/IGNACC_Controller` | `~/.ssh/id_ed25519_geolcost` |

---

## The idea in one minute

GitHub recognises **who you are by the SSH key**, not by your Git name or
email. One key can belong to only one account. So:

1. Each account gets **its own SSH key**.
2. `~/.ssh/config` gives each account a **host alias**: `github.com` for one,
   `github-geolcost` for the other. Both point to the real github.com, but
   with different keys.
3. The **remote URL** decides which alias (and so which account) Git uses:
   - `git@github.com:z3n-k3rn3l/...` uses the z3n-k3rn3l key
   - `git@github-geolcost:GeoLCost/...` uses the GeoLCost key

Your commit author name and email (`.gitconfig`) are a separate thing. They only
label commits and have nothing to do with which account can push.

---

## Step 1: One SSH key per account

Skip this if the keys already exist. The command is the same on Windows
(PowerShell) and Linux:

```bash
ssh-keygen -t ed25519 -C "geolcost@gmail.com" -f ~/.ssh/id_ed25519_geolcost
```

- `-t ed25519`: modern, short, secure key type.
- `-C "..."`: a comment so you can tell keys apart later. Any text works; the
  email is just a reminder.
- `-f ...`: the file name. Give each account's key a distinct name, or you'll
  overwrite the other one.

You'll be asked for a passphrase. Leave it empty for convenience, or set one
for safety (then see *Passphrases* below).

This creates two files:
- `id_ed25519_geolcost`: the **private** key. Never share it.
- `id_ed25519_geolcost.pub`: the **public** key. This goes to GitHub.

**Add the public key to the right account.** Sign in as that account, open
GitHub → **Settings → SSH and GPG keys → New SSH key**, and paste the contents
of the `.pub` file:

```bash
cat ~/.ssh/id_ed25519_geolcost.pub
```

On Windows, `type $env:USERPROFILE\.ssh\id_ed25519_geolcost.pub` does the same.

---

## Step 2: Lock down the key files

OpenSSH refuses to use a private key that other users can read. You'll see
`UNPROTECTED PRIVATE KEY FILE` or `Bad permissions`.

### Windows

On this PC the files had inherited access for *SYSTEM*, *Administrators* and
*OWNER RIGHTS*, so Windows OpenSSH ignored the keys. The fix gives only your own
user access. Run it for each private key **and** for `~/.ssh/config`:

```powershell
icacls $env:USERPROFILE\.ssh\id_ed25519_geolcost /grant:r "${env:USERDOMAIN}\${env:USERNAME}:F"
icacls $env:USERPROFILE\.ssh\id_ed25519_geolcost /inheritance:r
```

- `/grant:r "...:F"` gives your user Full control. `:r` replaces any existing
  entry for you instead of adding to it. The `${...}` braces keep PowerShell
  from reading `:F` as part of the variable name.
- `/inheritance:r` removes the permissions inherited from the `.ssh` folder,
  which is where the extra users came from.

Do the grant first, then remove inheritance. The other order can lock you out
of your own file.

Check the result. Only your user should be listed:

```powershell
icacls $env:USERPROFILE\.ssh\id_ed25519_geolcost
```
```
C:\Users\ramna\.ssh\id_ed25519_geolcost DEVWINSYS\ramna:(F)
```

### Linux

```bash
chmod 700 ~/.ssh
chmod 600 ~/.ssh/id_ed25519 ~/.ssh/id_ed25519_geolcost ~/.ssh/config
chmod 644 ~/.ssh/*.pub
```

- `700` on the folder: only you can enter it.
- `600` on private keys and config: only you can read or write them.
- `644` on public keys: anyone may read them, and that's fine.

---

## Step 3: `~/.ssh/config`, which key for which account

This file is identical on Windows (`C:\Users\<you>\.ssh\config`) and Linux
(`/home/<you>/.ssh/config`). This is the one used here:

```sshconfig
# Two GitHub accounts on one PC: pick the account by the host name in the remote URL.

# Default account (z3n-k3rn3l):  git@github.com:<owner>/<repo>.git
Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519
    IdentitiesOnly yes

# GeoLCost account:  git@github-geolcost:<owner>/<repo>.git
Host github-geolcost
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519_geolcost
    IdentitiesOnly yes
```

| Line | What it does |
|---|---|
| `Host github-geolcost` | The alias you write in remote URLs. It can be any name. |
| `HostName github.com` | The real server to connect to. |
| `User git` | GitHub's SSH user is always `git`. |
| `IdentityFile ...` | The key to offer for this alias. |
| `IdentitiesOnly yes` | Offer **only** that key. Without it, SSH may try your other key first. GitHub then logs you in as the wrong account, and you get "Repository not found" on a private repo. |

The plain `github.com` alias keeps the default account working everywhere
else, including old clones and copy-pasted GitHub URLs.

---

## Step 4: Test both accounts

```bash
ssh -T git@github.com
ssh -T git@github-geolcost
```

- `-T`: don't ask for a terminal, just log in and report.

Expected output:

```
Hi z3n-k3rn3l! You've successfully authenticated, but GitHub does not provide shell access.
Hi GeoLCost! You've successfully authenticated, but GitHub does not provide shell access.
```

The exit code is 1, which is normal: GitHub doesn't offer a shell. If the name
in the greeting is wrong, the wrong key is being used (see Troubleshooting).

---

## Step 5: `.gitconfig`, who the commits say they're from

The global identity on this PC (`C:\Users\ramna\.gitconfig`, or `~/.gitconfig`
on Linux):

```ini
[user]
	name = z3n_k3rn3l
	email = 75212700+z3n-k3rn3l@users.noreply.github.com
```

Set it with:

```bash
git config --global user.name "z3n_k3rn3l"
git config --global user.email "75212700+z3n-k3rn3l@users.noreply.github.com"
```

- `--global`: applies to every repo for your user.
- The `...@users.noreply.github.com` address is GitHub's private no-reply
  email. Commits link to your profile without exposing your real email. You
  find yours under GitHub → **Settings → Emails**.

In this project, **both repos show commits authored by z3n_k3rn3l**. That was a
deliberate choice, and it doesn't stop GeoLCost from pushing, because pushing
depends only on the SSH key.

### Optional: a different identity per folder

If you later want commits in some projects to say GeoLCost, Git can switch the
identity automatically based on the folder. Add this to the global
`.gitconfig`:

```ini
[includeIf "gitdir:~/work/geolcost/"]
	path = ~/.gitconfig-geolcost
```

And create `~/.gitconfig-geolcost`:

```ini
[user]
	name = GeoLCost
	email = geolcost@gmail.com
```

Every repo inside `~/work/geolcost/` now uses the GeoLCost identity; everything
else keeps the global one. The trailing `/` in `gitdir:` matters, because it
means "this folder and everything below it". On Windows you can write the path
as `gitdir:C:/Users/ramna/work/geolcost/`.

For a single repo, skip `--global`. Run this inside the repo:

```bash
git config user.name "GeoLCost"
git config user.email "geolcost@gmail.com"
```

---

## Step 6: Connect the repo to both accounts

Create the repo on GitHub under each account first: **private and empty**,
with no README, license or .gitignore. Otherwise the first push is rejected.

### Add the remotes

```bash
git remote add origin git@github-geolcost:GeoLCost/IGNACC_Controller.git
git remote add z3n git@github.com:z3n-k3rn3l/IGNACC_Controller.git
```

- `origin`: the main remote (GeoLCost here). Pulls come from it.
- `z3n`: a second remote, for the z3n-k3rn3l copy.

Check that each repo is reachable with the right key:

```bash
git ls-remote origin
git ls-remote z3n
```

An empty repo prints nothing and exits without error. "Repository not found"
means the key belongs to an account that can't see that private repo.

### First push

```bash
git push -u origin master
git push z3n master
```

- `-u`: remember `origin/master` as the default for `git push` and `git pull`
  on this branch. You only need it once.

### Make one `git push` update both

```bash
git remote set-url --add --push origin git@github-geolcost:GeoLCost/IGNACC_Controller.git
git remote set-url --add --push origin git@github.com:z3n-k3rn3l/IGNACC_Controller.git
```

- `--add --push` adds a **push-only** URL to `origin`. As soon as one push URL
  exists, Git pushes **only** to the push URLs. That's why the GeoLCost URL is
  added again, not just the z3n-k3rn3l one.
- Fetching and pulling still use the normal `url` (GeoLCost).

Confirm:

```bash
git remote -v
```
```
origin  git@github-geolcost:GeoLCost/IGNACC_Controller.git (fetch)
origin  git@github-geolcost:GeoLCost/IGNACC_Controller.git (push)
origin  git@github.com:z3n-k3rn3l/IGNACC_Controller.git (push)
z3n     git@github.com:z3n-k3rn3l/IGNACC_Controller.git (fetch)
z3n     git@github.com:z3n-k3rn3l/IGNACC_Controller.git (push)
```

The result in this repo's `.git/config`:

```ini
[remote "origin"]
	url = git@github-geolcost:GeoLCost/IGNACC_Controller.git
	fetch = +refs/heads/*:refs/remotes/origin/*
	pushurl = git@github-geolcost:GeoLCost/IGNACC_Controller.git
	pushurl = git@github.com:z3n-k3rn3l/IGNACC_Controller.git
[branch "master"]
	remote = origin
	merge = refs/heads/master
[remote "z3n"]
	url = git@github.com:z3n-k3rn3l/IGNACC_Controller.git
	fetch = +refs/heads/*:refs/remotes/z3n/*
```

---

## Everyday use

| You want to... | Command |
|---|---|
| Push to **both** accounts | `git push` |
| Check a push without sending anything | `git push --dry-run` |
| Push to z3n-k3rn3l only | `git push z3n master` |
| Push to GeoLCost only | `git push git@github-geolcost:GeoLCost/IGNACC_Controller.git master` |
| Pull (from GeoLCost) | `git pull` |
| Compare the two copies | `git fetch --all` then `git rev-parse origin/master z3n/master` (same hash twice = in sync) |
| Clone as GeoLCost on a new PC | `git clone git@github-geolcost:GeoLCost/IGNACC_Controller.git` |
| Clone as z3n-k3rn3l | `git clone git@github.com:z3n-k3rn3l/IGNACC_Controller.git` |

After a fresh clone on another machine, repeat the remote setup from Step 6
(remotes live in `.git/config`, which isn't part of the repo).

---

## Setting this up on Linux, start to finish

```bash
# 1. Keys (skip any that exist); add each .pub to its GitHub account
ssh-keygen -t ed25519 -C "z3n_k3rn3l@protonmail.com" -f ~/.ssh/id_ed25519
ssh-keygen -t ed25519 -C "geolcost@gmail.com" -f ~/.ssh/id_ed25519_geolcost

# 2. Permissions
chmod 700 ~/.ssh
chmod 600 ~/.ssh/id_ed25519 ~/.ssh/id_ed25519_geolcost

# 3. Paste the ~/.ssh/config from Step 3, then
chmod 600 ~/.ssh/config

# 4. Test
ssh -T git@github.com
ssh -T git@github-geolcost

# 5. Identity
git config --global user.name "z3n_k3rn3l"
git config --global user.email "75212700+z3n-k3rn3l@users.noreply.github.com"

# 6. Clone and wire both remotes
git clone git@github-geolcost:GeoLCost/IGNACC_Controller.git
cd IGNACC_Controller
git remote add z3n git@github.com:z3n-k3rn3l/IGNACC_Controller.git
git remote set-url --add --push origin git@github-geolcost:GeoLCost/IGNACC_Controller.git
git remote set-url --add --push origin git@github.com:z3n-k3rn3l/IGNACC_Controller.git
git push --dry-run
```

You can copy keys from Windows instead of making new ones. Copy both files of
each pair into `~/.ssh/` and fix the permissions with step 2. Windows files
often arrive with CRLF line endings. If SSH then says `invalid format`, run
`dos2unix ~/.ssh/id_ed25519*` on the private keys.

---

## Passphrases

If your keys have a passphrase, an agent saves you typing it on every push.

- **Linux:**
  ```bash
  eval "$(ssh-agent -s)"
  ssh-add ~/.ssh/id_ed25519 ~/.ssh/id_ed25519_geolcost
  ```
  Most desktop environments run an agent already; then only `ssh-add` is needed.
- **Windows** (PowerShell as Administrator, once):
  ```powershell
  Get-Service ssh-agent | Set-Service -StartupType Automatic
  Start-Service ssh-agent
  ```
  Then, as your normal user, `ssh-add $env:USERPROFILE\.ssh\id_ed25519_geolcost`.

`IdentitiesOnly yes` in the config still makes sure each alias uses only its
own key, even with both keys loaded in the agent.

---

## Troubleshooting

| What you see | Why | Fix |
|---|---|---|
| `UNPROTECTED PRIVATE KEY FILE` / `Bad permissions` | The key or config is readable by other users | Step 2 (Windows: `icacls`; Linux: `chmod 600`) |
| `Bad owner or permissions on .../.ssh/config` | Same, for the config file | Step 2 on `config` |
| `Permission denied (publickey)` | The key isn't on that GitHub account, or SSH didn't load it | Check Step 1; run `ssh -vT git@github-geolcost` to see which key is offered |
| `Hi <wrong account>!` | The other key is being used | Add `IdentitiesOnly yes`; check the alias in the remote URL |
| `ERROR: Repository not found` on a private repo | You're logged in as an account that can't see it | Use the matching alias in the URL (`github.com` vs `github-geolcost`) |
| Push rejected, "fetch first" | The GitHub repo isn't empty (it was created with a README) | Recreate it empty, or `git pull --rebase` from it first |
| Only one repo gets updated | `origin` lacks one of the push URLs | Re-run both `set-url --add --push` lines and check `git remote -v` |
| Commits show the wrong person | `user.name` / `user.email`, unrelated to keys | Step 5 |

`ssh -vT git@github-geolcost` (`-v` = verbose) prints each key SSH tries and
the config lines it applied. It's the quickest way to see what's going on.

---

## Undo

```bash
# Stop pushing to z3n-k3rn3l from plain `git push`
git remote set-url --delete --push origin git@github.com:z3n-k3rn3l/IGNACC_Controller.git

# Remove the second remote completely
git remote remove z3n
```

If you delete an alias from `~/.ssh/config`, update any remote URL that used it.
