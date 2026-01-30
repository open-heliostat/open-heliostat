<script lang="ts">
	import { onDestroy } from 'svelte';
	import SettingsCard from '$lib/components/SettingsCard.svelte';
	import GridForm from '$lib/components/GridForm.svelte';
	import Slider from '$lib/components/Slider.svelte';
	import Spinner from '$lib/components/Spinner.svelte';
	import { page } from '$app/stores';
	import { user } from '$lib/stores/user';

	export let restPath = '/rest/accelcalib';

	type AccelCalibState = {
		running: boolean;
		hasCalib: boolean;
		autoMove: boolean;
		useLimits: boolean;
		rms: number;
		sampleCount: number;
		sampleIntervalMs: number;
		settleMs: number;
		azSteps: number;
		elSteps: number;
		azMinDeg: number;
		azMaxDeg: number;
		elMinDeg: number;
		elMaxDeg: number;
		params: number[];
		paramsDeg?: {
			azOffsetDeg?: number;
			elOffsetDeg?: number;
			rollDeg?: number;
			pitchDeg?: number;
		};
	};

	let state: AccelCalibState;
	let pollId: any;

	async function getJsonRest<T>(path: string, data: T) {
		try {
			const response = await fetch(path, {
				method: 'GET',
				headers: {
					Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
					'Content-Type': 'application/json'
				}
			});
			data = await response.json();
		} catch (error) {
			console.error('Error:', error);
		}
		return data;
	}

	async function postJsonRest<T>(path: string, data: T) {
		try {
			const response = await fetch(path, {
				method: 'POST',
				headers: {
					Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
					'Content-Type': 'application/json'
				},
				body: JSON.stringify(data)
			});
			if (response.status == 200) {
				data = await response.json();
			} else {
				console.error('Wrong Path.', 3000);
			}
		} catch (error) {
			console.error('Error: ' + error, 3000);
		}
		return data;
	}

	async function refresh() {
		state = await getJsonRest(restPath, state as AccelCalibState);
		return state;
	}

	function start() {
		postJsonRest(restPath, { running: true }).then(() => refresh());
		pollId = setInterval(refresh, 1000);
	}

	function stop() {
		postJsonRest(restPath, { running: false }).then(() => refresh());
		clearInterval(pollId);
	}

	function capture() {
		postJsonRest(restPath, { capture: true }).then(() => refresh());
	}

	function reset() {
		postJsonRest(restPath, { reset: true }).then(() => refresh());
	}

	function solve() {
		postJsonRest(restPath, { solve: true }).then(() => refresh());
	}

	function apply() {
		postJsonRest(restPath, { apply: true }).then(() => refresh());
	}

	function updateInterval() {
		if (!state) return;
		postJsonRest(restPath, { sampleIntervalMs: state.sampleIntervalMs }).then(() => refresh());
	}

	function updateAutoMove() {
		if (!state) return;
		postJsonRest(restPath, { autoMove: state.autoMove }).then(() => refresh());
	}

	function updateUseLimits() {
		if (!state) return;
		postJsonRest(restPath, { useLimits: state.useLimits }).then(() => refresh());
	}

	function updateSettle() {
		if (!state) return;
		postJsonRest(restPath, { settleMs: state.settleMs }).then(() => refresh());
	}

	function updateSteps() {
		if (!state) return;
		postJsonRest(restPath, { azSteps: state.azSteps, elSteps: state.elSteps }).then(() => refresh());
	}

	function updateRanges() {
		if (!state) return;
		postJsonRest(restPath, {
			azMinDeg: state.azMinDeg,
			azMaxDeg: state.azMaxDeg,
			elMinDeg: state.elMinDeg,
			elMaxDeg: state.elMaxDeg
		}).then(() => refresh());
	}

	onDestroy(() => {
		clearInterval(pollId);
	});
</script>

<SettingsCard>
	{#snippet title()}
		<span>Accelerometer Calibration</span>
	{/snippet}
	{#await refresh()}
		<Spinner></Spinner>
	{:then}
		<div class="w-full">
			<div class="alert alert-info my-2 shadow">
				<span>
					Move the heliostat through wide azimuth/elevation ranges while sampling.
					Use Solve to compute offsets, then Apply to store them.
				</span>
			</div>
			<GridForm>
				<Slider
					label="Sample Interval (ms)"
					min={50}
					max={2000}
					step={10}
					bind:value={state.sampleIntervalMs}
					onChange={updateInterval}
				></Slider>
				<div class="form-control">
					<label class="label cursor-pointer">
						<span class="label-text">Auto-move</span>
						<input type="checkbox" class="toggle" bind:checked={state.autoMove} on:change={updateAutoMove} />
					</label>
				</div>
				<div class="form-control">
					<label class="label cursor-pointer">
						<span class="label-text">Use controller limits</span>
						<input type="checkbox" class="toggle" bind:checked={state.useLimits} on:change={updateUseLimits} />
					</label>
				</div>
				<Slider
					label="Settle Time (ms)"
					min={200}
					max={3000}
					step={50}
					bind:value={state.settleMs}
					onChange={updateSettle}
				></Slider>
				<Slider
					label="Az Steps"
					min={2}
					max={36}
					step={1}
					bind:value={state.azSteps}
					onChange={updateSteps}
				></Slider>
				<Slider
					label="El Steps"
					min={2}
					max={24}
					step={1}
					bind:value={state.elSteps}
					onChange={updateSteps}
				></Slider>
				{#if !state.useLimits}
					<Slider
						label="Az Min (deg)"
						min={0}
						max={360}
						step={1}
						bind:value={state.azMinDeg}
						onChange={updateRanges}
					></Slider>
					<Slider
						label="Az Max (deg)"
						min={0}
						max={360}
						step={1}
						bind:value={state.azMaxDeg}
						onChange={updateRanges}
					></Slider>
					<Slider
						label="El Min (deg)"
						min={-10}
						max={180}
						step={1}
						bind:value={state.elMinDeg}
						onChange={updateRanges}
					></Slider>
					<Slider
						label="El Max (deg)"
						min={-10}
						max={180}
						step={1}
						bind:value={state.elMaxDeg}
						onChange={updateRanges}
					></Slider>
				{/if}
			</GridForm>
			<div class="grid grid-cols-1 gap-2 md:grid-cols-2">
				<div class="stats shadow">
					<div class="stat">
						<div class="stat-title">Samples</div>
						<div class="stat-value text-lg">{state.sampleCount ?? 0}</div>
						<div class="stat-desc">RMS: {state.rms?.toFixed?.(3) ?? '—'}</div>
					</div>
				</div>
				<div class="stats shadow">
					<div class="stat">
						<div class="stat-title">Status</div>
						<div class="stat-value text-lg">{state.running ? 'Running' : 'Idle'}</div>
						<div class="stat-desc">Calibrated: {state.hasCalib ? 'Yes' : 'No'}</div>
					</div>
				</div>
			</div>

			<div class="mt-3 flex flex-row flex-wrap gap-2">
				<button class="btn btn-primary" on:click={start}>Start</button>
				<button class="btn btn-primary" on:click={stop}>Stop</button>
				<button class="btn" on:click={capture}>Capture</button>
				<button class="btn" on:click={solve}>Solve</button>
				<button class="btn btn-success" on:click={apply}>Apply</button>
				<div class="flex-grow"></div>
				<button class="btn btn-outline" on:click={reset}>Reset</button>
			</div>

			<div class="mt-4 overflow-x-auto">
				<table class="table table-sm">
					<thead>
						<tr>
							<th>Param</th>
							<th>Degrees</th>
						</tr>
					</thead>
					<tbody>
						<tr>
							<td>Azimuth Offset</td>
							<td>{state.paramsDeg?.azOffsetDeg?.toFixed?.(2) ?? '—'}</td>
						</tr>
						<tr>
							<td>Elevation Offset</td>
							<td>{state.paramsDeg?.elOffsetDeg?.toFixed?.(2) ?? '—'}</td>
						</tr>
						<tr>
							<td>Roll</td>
							<td>{state.paramsDeg?.rollDeg?.toFixed?.(2) ?? '—'}</td>
						</tr>
						<tr>
							<td>Pitch</td>
							<td>{state.paramsDeg?.pitchDeg?.toFixed?.(2) ?? '—'}</td>
						</tr>
					</tbody>
				</table>
			</div>
		</div>
	{/await}
</SettingsCard>
