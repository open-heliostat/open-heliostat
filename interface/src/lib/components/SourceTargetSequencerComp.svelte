<script lang="ts">
    import { onMount } from 'svelte';
    import SettingsCard from './SettingsCard.svelte';
    import Checkbox from './Checkbox.svelte';
    import Spinner from './Spinner.svelte';
    import SequencerChart, { type SequencerSeries } from './SequencerChart.svelte';
    import Select from './Select.svelte';
    import { getJsonRest, postJsonRest } from '$lib/stores/rest';
    import type { TargetSequencerState, SequencerKeyframe } from '$lib/types/models';

    export let label: string;
    export let restPath: string;

    let state: TargetSequencerState = { target: '', framesAz: [], framesEl: [], loop: false, running: false, duration: 0 };
    let loading = true;
    let targets: string[] = [];
    let activeSeriesId: string = 'azimuth';
    $: seriesData = seriesFromState(state.framesAz, state.framesEl);

    onMount(async () => {
        await Promise.all([refreshTargets(), refreshSequencer()]);
        loading = false;
    });

    async function refreshTargets() {
        const heliostat = await getJsonRest('/rest/heliostat', {} as any);
        const names = Object.keys(heliostat?.targetsMap ?? {});
        targets = names;
        if (!state.target && names.length) state = { ...state, target: names[0] };
    }

    function normalizeState(raw: any): TargetSequencerState {
        const framesObj = raw?.frames ?? {};
        const framesAz = (raw?.framesAz ?? framesObj.az ?? []) as SequencerKeyframe[];
        const framesEl = (raw?.framesEl ?? framesObj.el ?? []) as SequencerKeyframe[];
        return {
            target: raw?.target ?? state.target ?? '',
            framesAz: Array.isArray(framesAz) ? sortFrames(framesAz) : [],
            framesEl: Array.isArray(framesEl) ? sortFrames(framesEl) : [],
            loop: !!raw?.loop,
            running: !!raw?.running,
            duration: Number(raw?.duration) || 0,
        };
    }

    async function refreshSequencer() {
        const raw = await getJsonRest(restPath, state);
        state = normalizeState(raw);
    }

    async function setTarget(name: string) {
        state = { ...state, target: name };
        await postJsonRest(restPath, { target: name });
        await refreshSequencer();
    }

    function seriesFromState(framesAz: SequencerKeyframe[] = [], framesEl: SequencerKeyframe[] = []): SequencerSeries[] {
        // Keep a stable shape so the chart receives both series even if one is untouched
        return [
            {
                id: 'azimuth',
                label: 'Azimuth',
                color: '#38bdf8',
                frames: sortFrames(framesAz ?? [])
            },
            {
                id: 'elevation',
                label: 'Elevation',
                color: '#a855f7',
                frames: sortFrames(framesEl ?? [])
            }
        ];
    }

    function applySeries(series: SequencerSeries[]) {
        const az = series.find((s) => s.id === 'azimuth');
        const el = series.find((s) => s.id === 'elevation');
        state = {
            ...state,
            framesAz: az?.frames ?? [],
            framesEl: el?.frames ?? []
        };
    }

    function sortFrames(arr: SequencerKeyframe[]) {
        return arr.slice().sort((a, b) => a.tMs - b.tMs);
    }

    function addFrame(seriesId: 'azimuth' | 'elevation') {
        const list = seriesId === 'azimuth' ? state.framesAz : state.framesEl;
        const nextTime = list.length ? list[list.length - 1].tMs + 500 : 0;
        const next = [...list, { tMs: nextTime, value: 0 }];
        updateSeries(seriesId, sortFrames(next));
    }

    function removeFrame(seriesId: 'azimuth' | 'elevation', idx: number) {
        const list = seriesId === 'azimuth' ? state.framesAz : state.framesEl;
        const next = list.filter((_, i) => i !== idx);
        updateSeries(seriesId, sortFrames(next));
    }

    function updateSeries(seriesId: 'azimuth' | 'elevation', frames: SequencerKeyframe[]) {
        if (seriesId === 'azimuth') state = { ...state, framesAz: frames };
        else state = { ...state, framesEl: frames };
    }

    function setFrameAt(seriesId: 'azimuth' | 'elevation', idx: number, frame: SequencerKeyframe) {
        const list = seriesId === 'azimuth' ? state.framesAz : state.framesEl;
        const copy = list.slice();
        if (idx < copy.length) copy[idx] = frame;
        else copy.push(frame);
        updateSeries(seriesId, sortFrames(copy));
    }

    async function saveFrames(startAfter = false) {
        await postJsonRest(restPath, {
            target: state.target,
            frames: {
                az: state.framesAz.map((f) => ({ tMs: Number(f.tMs), value: Number(f.value) })),
                el: state.framesEl.map((f) => ({ tMs: Number(f.tMs), value: Number(f.value) })),
            },
            loop: state.loop,
            start: startAfter,
        });
        await refreshSequencer();
    }

    async function toggleLoop() {
        await postJsonRest(restPath, { loop: state.loop });
        await refreshSequencer();
    }

    async function start() {
        await postJsonRest(restPath, { start: true });
        await refreshSequencer();
    }

    async function stop() {
        await postJsonRest(restPath, { stop: true });
        await refreshSequencer();
    }

    async function clearFrames() {
        await postJsonRest(restPath, { clear: true });
        state = { ...state, framesAz: [], framesEl: [] };
        await refreshSequencer();
    }

    function onChartChange(e: CustomEvent<{ series: SequencerSeries[] } | { frames: SequencerKeyframe[] }>) {
        const detail = e.detail as any;
        if (detail.series) {
            // Merge into the existing pair so we never drop the other axis
            const next = seriesFromState(state.framesAz, state.framesEl);
            (detail.series as SequencerSeries[]).forEach((s) => {
                if (s.id === 'azimuth') next[0].frames = sortFrames(s.frames ?? []);
                if (s.id === 'elevation') next[1].frames = sortFrames(s.frames ?? []);
            });
            applySeries(next);
        } else if (detail.frames) {
            // fallback for single-series use
            state = { ...state, framesAz: detail.frames, framesEl: state.framesEl };
        }
    }
</script>

<SettingsCard>
    {#snippet title()}
        <span>{label} Target Sequencer</span>
    {/snippet}
    {#if loading}
        <Spinner />
    {:else}
        <div class="flex flex-col space-y-4">
            <div class="flex flex-row items-center gap-3">
                <div class="flex-grow"></div>
                <span class="badge badge-outline">{state.running ? 'Running' : 'Idle'}</span>
                <span class="badge badge-outline">Loop {state.loop ? 'on' : 'off'}</span>
                <span class="badge badge-outline">{state.duration} ms</span>
                <button class="btn btn-ghost btn-sm" on:click={() => { refreshTargets(); refreshSequencer(); }}>Refresh</button>
            </div>

            <div class="flex flex-row items-center gap-3">
                <label class="label cursor-pointer gap-2">
                    <span class="label-text">Edit</span>
                    <div class="btn-group">
                        <button class={`btn btn-sm ${activeSeriesId === 'azimuth' ? 'btn-active' : ''}`} on:click={() => activeSeriesId = 'azimuth'}>Azimuth</button>
                        <button class={`btn btn-sm ${activeSeriesId === 'elevation' ? 'btn-active' : ''}`} on:click={() => activeSeriesId = 'elevation'}>Elevation</button>
                    </div>
                </label>
            </div>

            <SequencerChart
                key={`${state.target}-${state.framesAz.length}-${state.framesEl.length}`}
                label={label}
                series={seriesData}
                activeSeriesId={activeSeriesId}
                running={state.running}
                loop={state.loop}
                on:change={onChartChange}
            />

            <Select label="Target" bind:value={state.target} onChange={(ev) => setTarget((ev.target as HTMLSelectElement).value)}>
                {#each targets as t}
                    <option value={t}>{t}</option>
                {/each}
            </Select>

            <p class="text-sm text-gray-500">Click points to drag. Right-click to delete. New points are added to the active series (Azimuth or Elevation).</p>

            <div class="overflow-x-auto">
                <table class="table table-zebra w-full text-sm">
                    <thead>
                        <tr>
                            <th class="w-16">#</th>
                            <th class="w-28">Time (ms)</th>
                            <th class="w-28">Azimuth</th>
                            <th class="w-28">Elevation</th>
                            <th class="w-16 text-right">Remove</th>
                        </tr>
                    </thead>
                    <tbody>
                        {#if state.framesAz.length === 0 && state.framesEl.length === 0}
                            <tr><td colspan="5" class="text-center">No keyframes</td></tr>
                        {:else}
                            {#each Array(Math.max((state.framesAz ?? []).length, (state.framesEl ?? []).length)) as _, idx}
                                <tr>
                                    <td>{idx + 1}</td>
                                    <td>
                                        <input class="input input-sm input-bordered w-full" type="number" min="0"
                                            value={(state.framesAz[idx]?.tMs ?? state.framesEl[idx]?.tMs) ?? 0}
                                            on:change={(e) => {
                                                const val = Number((e.target as HTMLInputElement).value);
                                                if (state.framesAz[idx]) setFrameAt('azimuth', idx, { ...state.framesAz[idx], tMs: val });
                                                if (state.framesEl[idx]) setFrameAt('elevation', idx, { ...state.framesEl[idx], tMs: val });
                                            }}
                                        />
                                    </td>
                                    <td>
                                        <input class="input input-sm input-bordered w-full" type="number" step="0.01"
                                            value={state.framesAz[idx]?.value ?? ''}
                                            on:change={(e) => {
                                                const val = Number((e.target as HTMLInputElement).value);
                                                const tMs = state.framesAz[idx]?.tMs ?? state.framesEl[idx]?.tMs ?? 0;
                                                setFrameAt('azimuth', idx, { tMs, value: val });
                                            }}
                                        />
                                    </td>
                                    <td>
                                        <input class="input input-sm input-bordered w-full" type="number" step="0.01"
                                            value={state.framesEl[idx]?.value ?? ''}
                                            on:change={(e) => {
                                                const val = Number((e.target as HTMLInputElement).value);
                                                const tMs = state.framesEl[idx]?.tMs ?? state.framesAz[idx]?.tMs ?? 0;
                                                setFrameAt('elevation', idx, { tMs, value: val });
                                            }}
                                        />
                                    </td>
                                    <td class="text-right">
                                        <button class="btn btn-xs btn-error" on:click={() => { removeFrame('azimuth', idx); removeFrame('elevation', idx); }}>X</button>
                                    </td>
                                </tr>
                            {/each}
                        {/if}
                    </tbody>
                </table>
            </div>

            <div class="flex flex-row items-center gap-3">
                <button class="btn btn-sm" on:click={() => addFrame('azimuth')}>Add az keyframe</button>
                <button class="btn btn-sm" on:click={() => addFrame('elevation')}>Add el keyframe</button>
            </div>

            <div class="flex flex-row items-center gap-3">
                <Checkbox label="Loop" bind:value={state.loop} onChange={toggleLoop} />
                <button class="btn btn-primary btn-sm" on:click={() => saveFrames(true)}>Load & Start</button>
                <button class="btn btn-secondary btn-sm" on:click={() => saveFrames(false)}>Load</button>
                <button class="btn btn-accent btn-sm" on:click={start}>Start</button>
                <button class="btn btn-outline btn-sm" on:click={stop}>Stop</button>
                <button class="btn btn-error btn-sm" on:click={clearFrames}>Clear</button>
            </div>
        </div>
    {/if}
</SettingsCard>
