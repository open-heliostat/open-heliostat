<script lang="ts">
    import { onMount } from 'svelte';
    import SettingsCard from './SettingsCard.svelte';
    import Checkbox from './Checkbox.svelte';
    import Spinner from './Spinner.svelte';
    import SequencerChart from './SequencerChart.svelte';
    import { getJsonRest, postJsonRest } from '$lib/stores/rest';
    import type { MovementSequencerState, SequencerKeyframe } from '$lib/types/models';

    export let label: string;
    export let restPath: string;

    let state: MovementSequencerState = { frames: [], loop: false, running: false, duration: 0 };
    let loading = true;

    onMount(async () => {
        loading = false;
        await refresh();
    });

    async function refresh() {
        state = await getJsonRest(restPath, state);
    }

    function addFrame() {
        const nextTime = state.frames.length > 0 ? state.frames[state.frames.length - 1].tMs + 500 : 0;
        state = { ...state, frames: [...state.frames, { tMs: nextTime, value: 0 }] };
    }

    function removeFrame(idx: number) {
        state = { ...state, frames: state.frames.filter((_, i) => i !== idx) };
    }

    function updateFrame(idx: number, frame: SequencerKeyframe) {
        const frames = state.frames.slice();
        frames[idx] = frame;
        const sorted = sortFrames(frames);
        state = { ...state, frames: sorted };
    }

    async function saveFrames(startAfter = false) {
        const frames = state.frames.map((f) => ({ tMs: Number(f.tMs), value: Number(f.value) }));
        await postJsonRest(restPath, { frames, loop: state.loop, start: startAfter });
        await refresh();
    }

    async function toggleLoop() {
        await postJsonRest(restPath, { loop: state.loop });
        await refresh();
    }

    async function start() {
        await postJsonRest(restPath, { start: true });
        await refresh();
    }

    async function stop() {
        await postJsonRest(restPath, { stop: true });
        await refresh();
    }

    async function clearFrames() {
        await postJsonRest(restPath, { clear: true });
        state = { ...state, frames: [] };
        await refresh();
    }

    function sortFrames(frames: SequencerKeyframe[]) {
        return frames.slice().sort((a, b) => a.tMs - b.tMs);
    }

    function onFramesChange(next: SequencerKeyframe[]) {
        const frames = sortFrames(next);
        state = { ...state, frames };
    }
</script>

<SettingsCard>
    {#snippet title()}
        <span>{label} Sequencer</span>
    {/snippet}
    {#if loading}
        <Spinner />
    {:else}
        <div class="flex flex-col space-y-4">
            <div class="flex flex-row items-center gap-3">
                <span class="badge badge-outline">{state.running ? 'Running' : 'Idle'}</span>
                <span class="badge badge-outline">Loop {state.loop ? 'on' : 'off'}</span>
                <span class="badge badge-outline">{state.duration} ms</span>
                <div class="flex-grow"></div>
                <button class="btn btn-ghost btn-sm" on:click={refresh}>Refresh</button>
            </div>
            <SequencerChart
                label={label}
                frames={state.frames}
                running={state.running}
                loop={state.loop}
                on:change={(e) => onFramesChange(e.detail.frames)}
            />
            <p class="text-sm text-gray-500">Click existing points to select, or click in empty space to add a new keyframe at that time/value.</p>
            <div class="flex flex-row items-center gap-3">
                <Checkbox label="Loop" bind:value={state.loop} onChange={toggleLoop} />
                <button class="btn btn-primary btn-sm" on:click={() => saveFrames(true)}>Load &amp; Start</button>
                <button class="btn btn-secondary btn-sm" on:click={() => saveFrames(false)}>Load</button>
                <button class="btn btn-accent btn-sm" on:click={start}>Start</button>
                <button class="btn btn-outline btn-sm" on:click={stop}>Stop</button>
                <button class="btn btn-error btn-sm" on:click={clearFrames}>Clear</button>
            </div>
            <div class="overflow-x-auto">
                <table class="table table-zebra w-full">
                    <thead>
                        <tr>
                            <th class="w-24">#</th>
                            <th class="w-32">Time (ms)</th>
                            <th class="w-32">Value</th>
                            <th class="w-16 text-right">Remove</th>
                        </tr>
                    </thead>
                    <tbody>
                        {#if state.frames.length === 0}
                            <tr><td colspan="4" class="text-center text-sm">No keyframes</td></tr>
                        {:else}
                            {#each state.frames as frame, idx}
                                <tr>
                                    <td>{idx + 1}</td>
                                    <td>
                                        <input class="input input-sm input-bordered w-full" type="number" min="0"
                                            bind:value={frame.tMs}
                                            on:change={(e) => updateFrame(idx, { ...frame, tMs: Number((e.target as HTMLInputElement).value) })}
                                        />
                                    </td>
                                    <td>
                                        <input class="input input-sm input-bordered w-full" type="number" step="0.01"
                                            bind:value={frame.value}
                                            on:change={(e) => updateFrame(idx, { ...frame, value: Number((e.target as HTMLInputElement).value) })}
                                        />
                                    </td>
                                    <td class="text-right">
                                        <button class="btn btn-xs btn-error" on:click={() => removeFrame(idx)}>X</button>
                                    </td>
                                </tr>
                            {/each}
                        {/if}
                    </tbody>
                </table>
            </div>
            <div class="flex flex-row items-center gap-3">
                <button class="btn btn-sm" on:click={addFrame}>Add keyframe</button>
            </div>
        </div>
    {/if}
</SettingsCard>
