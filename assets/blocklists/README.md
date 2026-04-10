# Block Lists

Block lists are plain-text domain lists used by ZAP Browser's ad/tracker blocker.

## Files

| File | Source | Purpose |
|---|---|---|
| `ads.txt` | EasyList | Advertisement domains |
| `trackers.txt` | EasyPrivacy | Tracking/analytics domains |

## Format

ZAP supports both formats:

**EasyList format:**
```
||ads.example.com^
||tracker.analytics.io^
```

**Hosts file format:**
```
0.0.0.0 ads.example.com
0.0.0.0 tracker.analytics.io
```

**Plain domain format (one per line):**
```
ads.example.com
tracker.analytics.io
```

## Updating

Block lists auto-refresh every 24 hours from:
- `https://easylist.to/easylist/easylist.txt`
- `https://easylist.to/easylist/easyprivacy.txt`

You can also manually replace these files with any compatible list.

## Seeding Initial Lists

Run the setup script to download initial block lists:

```powershell
.\scripts\download_blocklists.ps1
```

Or download manually:
- https://easylist.to/easylist/easylist.txt → save as `ads.txt`
- https://easylist.to/easylist/easyprivacy.txt → save as `trackers.txt`
