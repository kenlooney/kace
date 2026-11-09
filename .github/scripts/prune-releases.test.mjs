import assert from 'node:assert/strict';
import test from 'node:test';
import { releasesToPrune, runRetention } from './prune-releases.mjs';

function release(id, tag, prerelease, extra = {}) {
  return { id, tag_name: tag, prerelease, draft: false, immutable: false,
    published_at: `2026-11-${String(id).padStart(2, '0')}T00:00:00Z`, ...extra };
}

test('keeps five snapshots and three stable releases by publication time', () => {
  const snapshots = Array.from({ length: 7 }, (_, i) =>
    release(i + 1, `kace-dev-0.${i}.0-11092026`, true));
  const stable = Array.from({ length: 5 }, (_, i) =>
    release(i + 11, `kace-0.${i}.0-11092026`, false));
  const unmanaged = [
    release(20, 'manual-release', false),
    release(21, 'kace-dev-0.9.0-11092026', false),
    release(22, 'kace-0.9.0-11092026', true),
    release(23, 'kace-0.10.0-11092026', false, { draft: true }),
  ];
  assert.deepEqual(releasesToPrune([...stable, ...unmanaged, ...snapshots]).map(r => r.tag_name), [
    'kace-dev-0.1.0-11092026', 'kace-dev-0.0.0-11092026',
    'kace-0.1.0-11092026', 'kace-0.0.0-11092026',
  ]);
});

test('refuses to delete anything if an old managed release is immutable', () => {
  const snapshots = Array.from({ length: 6 }, (_, i) =>
    release(i + 1, `kace-dev-0.${i}.0-11092026`, true, { immutable: i === 0 }));
  assert.throws(() => releasesToPrune(snapshots), /immutable/);
});

test('keeps all releases when there are fewer than the limits', () => {
  assert.deepEqual(releasesToPrune([release(1, 'kace-0.1.0-11092026', false)]), []);
});

test('uses release id as a deterministic tie breaker', () => {
  const releases = Array.from({ length: 4 }, (_, i) =>
    release(i + 1, `kace-0.${i}.0-11092026`, false,
      { published_at: '2026-11-09T00:00:00Z' }));
  assert.deepEqual(releasesToPrune(releases).map(r => r.id), [1]);
});

test('requests all pages and deletes only the old managed release and tag', () => {
  const releases = Array.from({ length: 4 }, (_, i) =>
    release(i + 1, `kace-0.${i}.0-11092026`, false));
  const calls = [];
  runRetention('owner/repo', args => {
    calls.push(args);
    return args[0] === 'api' ? JSON.stringify([releases.slice(0, 2), releases.slice(2)]) : '';
  });
  assert.deepEqual(calls, [
    ['api', '--paginate', '--slurp', 'repos/owner/repo/releases?per_page=100'],
    ['release', 'delete', 'kace-0.0.0-11092026', '--yes', '--cleanup-tag'],
  ]);
});

test('does not delete releases if the API response is malformed', () => {
  const calls = [];
  assert.throws(() => runRetention('owner/repo', args => {
    calls.push(args);
    return JSON.stringify({ unexpected: true });
  }), /Unexpected release API response/);
  assert.equal(calls.length, 1);
});
