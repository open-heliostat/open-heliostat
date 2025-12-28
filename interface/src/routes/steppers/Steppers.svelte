<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import SettingsCard from '$lib/components/SettingsCard.svelte';
	import StepperRestComp from '$lib/components/StepperRestComp.svelte';

	onMount(() => {
	});

	onDestroy(() => {
	});

</script>

<SettingsCard>
	{#snippet title()}
		<span>Azimuth Stepper</span>
	{/snippet}
	<StepperRestComp restPath={"/rest/heliostat/azimuth/stepper"}></StepperRestComp>
</SettingsCard>
<SettingsCard>
	{#snippet title()}
		<span>Elevation Stepper</span>
	{/snippet}
	<StepperRestComp restPath={"/rest/heliostat/elevation/stepper"}></StepperRestComp>
</SettingsCard>