import { spawnSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

const version = '[0-9]+(?:\\.[0-9]+){0,3}';
const snapshotTag = new RegExp(`^kace-dev-${version}-[0-9]{8}$`);
const stableTag = new RegExp(`^kace-${version}-[0-9]{8}$`);

// Only releases created by this workflow are eligible; leave drafts and
// unrelated or mismatched release types alone.
export function releasesToPrune(releases) {
  const candidates = (pattern, prerelease, keep) => releases
    .filter(release => !release.draft && release.prerelease === prerelease &&
      pattern.test(release.tag_name) && release.published_at)
    .sort((a, b) => b.published_at.localeCompare(a.published_at) || b.id - a.id)
    .slice(keep);

  const old = [...candidates(snapshotTag, true, 4), ...candidates(stableTag, false, 4)];
  const immutable = old.filter(release => release.immutable);
  if (immutable.length) {
    throw new Error(`Cannot prune immutable releases: ${immutable.map(release => release.tag_name).join(', ')}`);
  }
  return old;
}

function runGh(args) {
  const result = spawnSync('gh', args, { encoding: 'utf8', maxBuffer: 32 * 1024 * 1024 });
  if (result.error || result.status !== 0) {
    throw new Error(`gh ${args.join(' ')} failed: ${result.error?.message ?? result.stderr}`);
  }
  return result.stdout;
}

export function runRetention(repo, gh = runGh) {
  if (!repo) throw new Error('GH_REPO is required');
  const pages = JSON.parse(gh(['api', '--paginate', '--slurp',
    `repos/${repo}/releases?per_page=100`]));
  if (!Array.isArray(pages) || pages.some(page => !Array.isArray(page))) {
    throw new Error('Unexpected release API response');
  }
  const old = releasesToPrune(pages.flat());
  for (const release of old) {
    console.log(`Deleting old release and tag: ${release.tag_name}`);
    gh(['release', 'delete', release.tag_name, '--yes', '--cleanup-tag']);
  }
  console.log(`Release retention complete: removed ${old.length} old release(s).`);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  try {
    runRetention(process.env.GH_REPO);
  } catch (error) {
    console.error(`Release retention failed: ${error.message}`);
    process.exitCode = 1;
  }
}
